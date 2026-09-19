#include "LoadBalancer.h"

// ============================================================
// LoadBalancer: construction/teardown, logging, socket config,
// backend connection + health checking, and small I/O helpers.
// ============================================================

// ------------------------------------------------------------
// CONSTRUCTOR
// ------------------------------------------------------------

LoadBalancer::LoadBalancer()
    {
        backends.emplace_back(
            make_unique<BackendServer>(
                "127.0.0.1",
                8080
            )
        );

        backends.emplace_back(
            make_unique<BackendServer>(
                "127.0.0.1",
                8000
            )
        );

        backends.emplace_back(
            make_unique<BackendServer>(
                "127.0.0.1",
                9000
            )
        );
    }



// ------------------------------------------------------------
// DESTRUCTOR
// ------------------------------------------------------------

LoadBalancer::~LoadBalancer()
    {
        running = false;

        queueCV.notify_all();

        for(auto& worker : workerThreads)
        {
            if(worker.joinable())
            {
                worker.join();
            }
        }

        if(listenSocket != INVALID_SOCKET)
        {
            closesocket(listenSocket);
            listenSocket = INVALID_SOCKET;
        }

        WSACleanup();
    }



// ------------------------------------------------------------
// LOG
// ------------------------------------------------------------

void LoadBalancer::log(const string& message)
    {
        lock_guard<mutex> lock(logMutex);

        cout << message << endl;
    }



// ------------------------------------------------------------
// CONFIGURE SOCKET
// ------------------------------------------------------------

void LoadBalancer::configureSocket(SOCKET sock)
    {
        int receiveBuffer = 1024 * 1024;

        setsockopt(
            sock,
            SOL_SOCKET,
            SO_RCVBUF,
            reinterpret_cast<char*>(&receiveBuffer),
            sizeof(receiveBuffer)
        );


        int sendBuffer = 1024 * 1024;

        setsockopt(
            sock,
            SOL_SOCKET,
            SO_SNDBUF,
            reinterpret_cast<char*>(&sendBuffer),
            sizeof(sendBuffer)
        );


        int timeout = SOCKET_TIMEOUT_MS;

        setsockopt(
            sock,
            SOL_SOCKET,
            SO_RCVTIMEO,
            reinterpret_cast<char*>(&timeout),
            sizeof(timeout)
        );


        setsockopt(
            sock,
            SOL_SOCKET,
            SO_SNDTIMEO,
            reinterpret_cast<char*>(&timeout),
            sizeof(timeout)
        );


        BOOL noDelay = TRUE;

        setsockopt(
            sock,
            IPPROTO_TCP,
            TCP_NODELAY,
            reinterpret_cast<char*>(&noDelay),
            sizeof(noDelay)
        );


        BOOL keepAlive = TRUE;

        setsockopt(
            sock,
            SOL_SOCKET,
            SO_KEEPALIVE,
            reinterpret_cast<char*>(&keepAlive),
            sizeof(keepAlive)
        );
    }



// ------------------------------------------------------------
// CONNECT TO BACKEND
// ------------------------------------------------------------

SOCKET LoadBalancer::connectToBackend(
        BackendServer& backend
    )
    {
        SOCKET sock = socket(
            AF_INET,
            SOCK_STREAM,
            IPPROTO_TCP
        );


        if(sock == INVALID_SOCKET)
        {
            return INVALID_SOCKET;
        }


        configureSocket(sock);


        // ----------------------------------------------------
        // Non-blocking mode
        // ----------------------------------------------------

        u_long mode = 1;

        if(
            ioctlsocket(
                sock,
                FIONBIO,
                &mode
            ) != 0
        )
        {
            closesocket(sock);

            return INVALID_SOCKET;
        }


        // ----------------------------------------------------
        // Backend address
        // ----------------------------------------------------

        sockaddr_in serverAddress{};

        serverAddress.sin_family = AF_INET;

        serverAddress.sin_port =
            htons(
                static_cast<u_short>(
                    backend.port
                )
            );


        if(
            inet_pton(
                AF_INET,
                backend.host.c_str(),
                &serverAddress.sin_addr
            ) != 1
        )
        {
            closesocket(sock);

            return INVALID_SOCKET;
        }


        // ----------------------------------------------------
        // Connect
        // ----------------------------------------------------

        int result = connect(
            sock,
            reinterpret_cast<sockaddr*>(
                &serverAddress
            ),
            sizeof(serverAddress)
        );


        if(result == SOCKET_ERROR)
        {
            int error = WSAGetLastError();


            if(
                error != WSAEWOULDBLOCK &&
                error != WSAEINPROGRESS &&
                error != WSAEALREADY
            )
            {
                closesocket(sock);

                return INVALID_SOCKET;
            }


            // ------------------------------------------------
            // Wait for connection
            // ------------------------------------------------

            fd_set writeSet;

            FD_ZERO(&writeSet);

            FD_SET(
                sock,
                &writeSet
            );


            timeval timeout{};

            timeout.tv_sec =
                CONNECT_TIMEOUT_MS / 1000;

            timeout.tv_usec =
                (CONNECT_TIMEOUT_MS % 1000) * 1000;


            result = select(
                0,
                nullptr,
                &writeSet,
                nullptr,
                &timeout
            );


            if(result <= 0)
            {
                closesocket(sock);

                return INVALID_SOCKET;
            }


            // ------------------------------------------------
            // Check connection result
            // ------------------------------------------------

            int socketError = 0;

            int errorLength =
                sizeof(socketError);


            getsockopt(
                sock,
                SOL_SOCKET,
                SO_ERROR,
                reinterpret_cast<char*>(
                    &socketError
                ),
                &errorLength
            );


            if(socketError != 0)
            {
                closesocket(sock);

                return INVALID_SOCKET;
            }
        }


        // ----------------------------------------------------
        // Back to blocking mode
        // ----------------------------------------------------

        mode = 0;

        ioctlsocket(
            sock,
            FIONBIO,
            &mode
        );


        return sock;
    }



// ------------------------------------------------------------
// HEALTH CHECK
// ------------------------------------------------------------

bool LoadBalancer::checkBackend(
        BackendServer& backend
    )
    {
        SOCKET sock =
            connectToBackend(backend);


        if(sock == INVALID_SOCKET)
        {
            backend.healthy = false;

            return false;
        }


        closesocket(sock);

        backend.healthy = true;

        return true;
    }



// ------------------------------------------------------------
// HEALTH CHECK THREAD
// ------------------------------------------------------------

void LoadBalancer::healthChecker()
    {
        while(running)
        {
            for(auto& backendPtr : backends)
            {
                if(!backendPtr)
                {
                    continue;
                }


                BackendServer& backend =
                    *backendPtr;


                bool previousStatus =
                    backend.healthy.load();


                bool currentStatus =
                    checkBackend(backend);


                // ------------------------------------------------
                // Backend recovered
                // ------------------------------------------------

                if(
                    currentStatus &&
                    !previousStatus
                )
                {
                    log(
                        "[HEALTH] Backend recovered: " +
                        backend.host +
                        ":" +
                        to_string(
                            backend.port
                        )
                    );
                }


                // ------------------------------------------------
                // Backend failed
                // ------------------------------------------------

                if(
                    !currentStatus &&
                    previousStatus
                )
                {
                    log(
                        "[HEALTH] Backend unavailable: " +
                        backend.host +
                        ":" +
                        to_string(
                            backend.port
                        )
                    );
                }
            }


            this_thread::sleep_for(
                chrono::milliseconds(
                    HEALTH_CHECK_INTERVAL_MS
                )
            );
        }
    }



// ------------------------------------------------------------
// GET BACKEND ORDER (least-connections)
// ------------------------------------------------------------

vector<int> LoadBalancer::getBackendOrder()
    {
        vector<int> indexes;


        // ----------------------------------------------------
        // First collect healthy backends
        // ----------------------------------------------------

        for(
            int i = 0;
            i < static_cast<int>(
                backends.size()
            );
            i++
        )
        {
            if(
                backends[i] &&
                backends[i]->healthy.load()
            )
            {
                indexes.push_back(i);
            }
        }


        // ----------------------------------------------------
        // Sort by active connections
        // ----------------------------------------------------

        sort(
            indexes.begin(),
            indexes.end(),
            [&](int a, int b)
            {
                if(
                    a < 0 ||
                    b < 0 ||
                    a >= static_cast<int>(backends.size()) ||
                    b >= static_cast<int>(backends.size())
                )
                {
                    return false;
                }

                return
                    backends[a]->activeConnections.load()
                    <
                    backends[b]->activeConnections.load();
            }
        );


        return indexes;
    }



// ------------------------------------------------------------
// SEND ALL
// ------------------------------------------------------------

bool LoadBalancer::sendAll(
        SOCKET sock,
        const char* data,
        int length
    )
    {
        int totalSent = 0;


        while(totalSent < length)
        {
            int sent = send(
                sock,
                data + totalSent,
                length - totalSent,
                0
            );


            if(sent == SOCKET_ERROR)
            {
                return false;
            }


            if(sent == 0)
            {
                return false;
            }


            totalSent += sent;
        }


        return true;
    }



// ------------------------------------------------------------
// SEND 503 RESPONSE
// ------------------------------------------------------------

void LoadBalancer::send503(
        SOCKET client
    )
    {
        const string body =
            R"({"error":"No backend available"})";


        string response =
            "HTTP/1.1 503 Service Unavailable\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " +
            to_string(body.size()) +
            "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            body;


        sendAll(
            client,
            response.c_str(),
            static_cast<int>(
                response.size()
            )
        );
    }

