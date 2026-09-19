#include "LoadBalancer.h"

// ============================================================
// LoadBalancer startup: winsock init, bind/listen, and the
// main accept loop that feeds the client queue.
// ============================================================

// ------------------------------------------------------------
// INITIALIZE
// ------------------------------------------------------------

bool LoadBalancer::initialize()
    {
        WSADATA wsaData{};


        int result =
            WSAStartup(
                MAKEWORD(2, 2),
                &wsaData
            );


        if(result != 0)
        {
            cerr
                << "WSAStartup failed"
                << endl;

            return false;
        }


        // ----------------------------------------------------
        // Create socket
        // ----------------------------------------------------

        listenSocket =
            socket(
                AF_INET,
                SOCK_STREAM,
                IPPROTO_TCP
            );


        if(
            listenSocket ==
            INVALID_SOCKET
        )
        {
            cerr
                << "Failed to create socket"
                << endl;


            WSACleanup();

            return false;
        }


        // ----------------------------------------------------
        // SO_REUSEADDR
        // ----------------------------------------------------

        BOOL reuseAddress =
            TRUE;


        setsockopt(
            listenSocket,
            SOL_SOCKET,
            SO_REUSEADDR,
            reinterpret_cast<char*>(
                &reuseAddress
            ),
            sizeof(reuseAddress)
        );


        configureSocket(
            listenSocket
        );


        // ----------------------------------------------------
        // Bind
        // ----------------------------------------------------

        sockaddr_in address{};


        address.sin_family =
            AF_INET;


        address.sin_addr.s_addr =
            htonl(INADDR_ANY);


        address.sin_port =
            htons(
                LOAD_BALANCER_PORT
            );


        if(
            bind(
                listenSocket,
                reinterpret_cast<sockaddr*>(
                    &address
                ),
                sizeof(address)
            )
            == SOCKET_ERROR
        )
        {
            cerr
                << "Bind failed on port "
                << LOAD_BALANCER_PORT
                << endl;


            closesocket(
                listenSocket
            );


            WSACleanup();

            return false;
        }


        // ----------------------------------------------------
        // Listen
        // ----------------------------------------------------

        if(
            listen(
                listenSocket,
                BACKLOG
            )
            == SOCKET_ERROR
        )
        {
            cerr
                << "Listen failed"
                << endl;


            closesocket(
                listenSocket
            );


            WSACleanup();

            return false;
        }


        return true;
    }



// ------------------------------------------------------------
// START
// ------------------------------------------------------------

void LoadBalancer::start()
    {
        if(!initialize())
        {
            return;
        }


        cout << endl;

        cout
            << "=========================================="
            << endl;

        cout
            << "          C++ LOAD BALANCER"
            << endl;

        cout
            << "=========================================="
            << endl;


        cout << endl;


        cout
            << "Load Balancer : 127.0.0.1:"
            << LOAD_BALANCER_PORT
            << endl;


        cout << endl;


        cout
            << "Backends:"
            << endl;


        for(auto& backendPtr : backends)
        {
            if(!backendPtr)
            {
                continue;
            }


            BackendServer& backend =
                *backendPtr;


            cout
                << "  -> "
                << backend.host
                << ":"
                << backend.port
                << endl;
        }


        cout << endl;


        cout
            << "Algorithm      : Least Connections"
            << endl;

        cout
            << "Health Check   : "
            << HEALTH_CHECK_INTERVAL_MS
            << " ms"
            << endl;

        cout
            << "Connect Timeout: "
            << CONNECT_TIMEOUT_MS
            << " ms"
            << endl;

        cout
            << "Socket Timeout : "
            << SOCKET_TIMEOUT_MS
            << " ms"
            << endl;

        cout
            << "Worker Threads : "
            << WORKER_THREADS
            << endl;


        cout
            << "=========================================="
            << endl;


        cout << endl;


        // ----------------------------------------------------
        // Start health checker
        // ----------------------------------------------------

        thread healthThread(
            &LoadBalancer::healthChecker,
            this
        );


        // ----------------------------------------------------
        // Start worker threads
        // ----------------------------------------------------

        startWorkers();


        // ----------------------------------------------------
        // Accept clients
        // ----------------------------------------------------

        while(running)
        {
            sockaddr_in clientAddress{};


            int clientAddressLength =
                sizeof(clientAddress);


            SOCKET client =
                accept(
                    listenSocket,
                    reinterpret_cast<sockaddr*>(
                        &clientAddress
                    ),
                    &clientAddressLength
                );


            if(
                client ==
                INVALID_SOCKET
            )
            {
                if(!running)
                {
                    break;
                }


                continue;
            }


            // ------------------------------------------------
            // Put client into queue
            // ------------------------------------------------

            {
                lock_guard<mutex> lock(
                    queueMutex
                );


                clientQueue.push(client);
            }


            queueCV.notify_one();
        }


        // ----------------------------------------------------
        // Stop server
        // ----------------------------------------------------

        running = false;


        queueCV.notify_all();


        // ----------------------------------------------------
        // Wait for health checker
        // ----------------------------------------------------

        if(
            healthThread.joinable()
        )
        {
            healthThread.join();
        }


        // ----------------------------------------------------
        // Wait for workers
        // ----------------------------------------------------

        for(auto& worker : workerThreads)
        {
            if(worker.joinable())
            {
                worker.join();
            }
        }


        // ----------------------------------------------------
        // Close listening socket
        // ----------------------------------------------------

        if(
            listenSocket !=
            INVALID_SOCKET
        )
        {
            closesocket(
                listenSocket
            );

            listenSocket =
                INVALID_SOCKET;
        }
    }

