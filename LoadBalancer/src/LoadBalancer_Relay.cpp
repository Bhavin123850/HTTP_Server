#include "LoadBalancer.h"

// ============================================================
// LoadBalancer relay + client-handling: the raw TCP pump
// between client and backend, per-client dispatch, and the
// worker-thread pool that consumes the client queue.
// ============================================================

// ------------------------------------------------------------
// RELAY TCP DATA
// ------------------------------------------------------------

void LoadBalancer::relay(
        SOCKET client,
        SOCKET backend
    )
    {
        char buffer[BUFFER_SIZE];


        bool clientOpen = true;

        bool backendOpen = true;


        while(
            clientOpen ||
            backendOpen
        )
        {
            fd_set readSet;

            FD_ZERO(&readSet);


            if(clientOpen)
            {
                FD_SET(
                    client,
                    &readSet
                );
            }


            if(backendOpen)
            {
                FD_SET(
                    backend,
                    &readSet
                );
            }


            timeval timeout{};

            timeout.tv_sec = 30;

            timeout.tv_usec = 0;


            int result = select(
                0,
                &readSet,
                nullptr,
                nullptr,
                &timeout
            );


            // ------------------------------------------------
            // Timeout
            // ------------------------------------------------

            if(result == 0)
            {
                break;
            }


            // ------------------------------------------------
            // select error
            // ------------------------------------------------

            if(result == SOCKET_ERROR)
            {
                break;
            }


            // =================================================
            // CLIENT -> BACKEND
            // =================================================

            if(
                clientOpen &&
                FD_ISSET(
                    client,
                    &readSet
                )
            )
            {
                int received =
                    recv(
                        client,
                        buffer,
                        BUFFER_SIZE,
                        0
                    );


                if(received == 0)
                {
                    clientOpen = false;


                    shutdown(
                        backend,
                        SD_SEND
                    );
                }
                else if(received == SOCKET_ERROR)
                {
                    clientOpen = false;


                    shutdown(
                        backend,
                        SD_SEND
                    );
                }
                else
                {
                    bool success =
                        sendAll(
                            backend,
                            buffer,
                            received
                        );


                    if(!success)
                    {
                        clientOpen = false;
                    }
                }
            }


            // =================================================
            // BACKEND -> CLIENT
            // =================================================

            if(
                backendOpen &&
                FD_ISSET(
                    backend,
                    &readSet
                )
            )
            {
                int received =
                    recv(
                        backend,
                        buffer,
                        BUFFER_SIZE,
                        0
                    );


                if(received == 0)
                {
                    backendOpen = false;


                    shutdown(
                        client,
                        SD_SEND
                    );
                }
                else if(received == SOCKET_ERROR)
                {
                    backendOpen = false;


                    shutdown(
                        client,
                        SD_SEND
                    );
                }
                else
                {
                    bool success =
                        sendAll(
                            client,
                            buffer,
                            received
                        );


                    if(!success)
                    {
                        backendOpen = false;
                    }
                }
            }
        }
    }



// ------------------------------------------------------------
// HANDLE CLIENT
// ------------------------------------------------------------

void LoadBalancer::handleClient(
        SOCKET client
    )
    {
        configureSocket(client);


        SOCKET backendSocket =
            INVALID_SOCKET;

        int selectedBackend = -1;


        // ----------------------------------------------------
        // Get healthy backends
        // ----------------------------------------------------

        vector<int> backendOrder =
            getBackendOrder();


        // ----------------------------------------------------
        // If all unhealthy, try all backends
        // ----------------------------------------------------

        if(backendOrder.empty())
        {
            for(
                int i = 0;
                i < static_cast<int>(
                    backends.size()
                );
                i++
            )
            {
                if(backends[i])
                {
                    backendOrder.push_back(i);
                }
            }
        }


        // ----------------------------------------------------
        // Try backends
        // ----------------------------------------------------

        for(int index : backendOrder)
        {
            // IMPORTANT:
            // Never access an invalid vector index.

            if(
                index < 0 ||
                index >= static_cast<int>(
                    backends.size()
                )
            )
            {
                continue;
            }


            if(!backends[index])
            {
                continue;
            }


            BackendServer& backend =
                *backends[index];


            SOCKET candidate =
                connectToBackend(
                    backend
                );


            if(
                candidate !=
                INVALID_SOCKET
            )
            {
                backendSocket =
                    candidate;


                selectedBackend =
                    index;


                backend.healthy =
                    true;


                break;
            }


            backend.healthy =
                false;
        }


        // ----------------------------------------------------
        // Emergency retry
        // ----------------------------------------------------

        if(
            backendSocket ==
            INVALID_SOCKET
        )
        {
            for(
                int i = 0;
                i < static_cast<int>(
                    backends.size()
                );
                i++
            )
            {
                if(!backends[i])
                {
                    continue;
                }


                bool alreadyTried =
                    find(
                        backendOrder.begin(),
                        backendOrder.end(),
                        i
                    )
                    !=
                    backendOrder.end();


                if(alreadyTried)
                {
                    continue;
                }


                BackendServer& backend =
                    *backends[i];


                SOCKET candidate =
                    connectToBackend(
                        backend
                    );


                if(
                    candidate !=
                    INVALID_SOCKET
                )
                {
                    backendSocket =
                        candidate;


                    selectedBackend =
                        i;


                    backend.healthy =
                        true;


                    break;
                }
            }
        }


        // ----------------------------------------------------
        // No backend
        // ----------------------------------------------------

        if(
            backendSocket ==
            INVALID_SOCKET
        )
        {
            log(
                "[LB] No backend available"
            );


            send503(client);


            shutdown(
                client,
                SD_BOTH
            );


            closesocket(client);

            return;
        }


        // ----------------------------------------------------
        // FINAL SAFETY CHECK
        // ----------------------------------------------------

        if(
            selectedBackend < 0 ||
            selectedBackend >=
                static_cast<int>(
                    backends.size()
                ) ||
            !backends[selectedBackend]
        )
        {
            closesocket(backendSocket);

            send503(client);

            shutdown(
                client,
                SD_BOTH
            );

            closesocket(client);

            return;
        }


        BackendServer& backend =
            *backends[selectedBackend];


        // ----------------------------------------------------
        // Increase active connection count
        // ----------------------------------------------------

        backend.activeConnections.fetch_add(1);


        log(
            "[LB] Client -> " +
            to_string(
                backend.port
            ) +
            " | Active: " +
            to_string(
                backend.activeConnections.load()
            )
        );


        // ----------------------------------------------------
        // Relay traffic
        // ----------------------------------------------------

        relay(
            client,
            backendSocket
        );


        // ----------------------------------------------------
        // Cleanup backend
        // ----------------------------------------------------

        shutdown(
            backendSocket,
            SD_BOTH
        );


        closesocket(
            backendSocket
        );


        // ----------------------------------------------------
        // Cleanup client
        // ----------------------------------------------------

        shutdown(
            client,
            SD_BOTH
        );


        closesocket(
            client
        );


        // ----------------------------------------------------
        // Decrease active count
        // ----------------------------------------------------

        backend.activeConnections.fetch_sub(1);


        log(
            "[LB] Client disconnected from " +
            to_string(
                backend.port
            ) +
            " | Active: " +
            to_string(
                backend.activeConnections.load()
            )
        );
    }



// ------------------------------------------------------------
// WORKER THREAD
// ------------------------------------------------------------

void LoadBalancer::worker()
    {
        while(true)
        {
            SOCKET client =
                INVALID_SOCKET;


            // ------------------------------------------------
            // Get client from queue
            // ------------------------------------------------

            {
                unique_lock<mutex> lock(
                    queueMutex
                );


                queueCV.wait(
                    lock,
                    [&]()
                    {
                        return
                            !running ||
                            !clientQueue.empty();
                    }
                );


                // If server stopped and queue is empty
                if(
                    !running &&
                    clientQueue.empty()
                )
                {
                    return;
                }


                if(clientQueue.empty())
                {
                    continue;
                }


                client =
                    clientQueue.front();


                clientQueue.pop();
            }


            // ------------------------------------------------
            // Handle client
            // ------------------------------------------------

            try
            {
                handleClient(client);
            }
            catch(const exception& e)
            {
                log(
                    string("[LB] Client error: ") +
                    e.what()
                );


                if(client != INVALID_SOCKET)
                {
                    shutdown(
                        client,
                        SD_BOTH
                    );

                    closesocket(client);
                }
            }
            catch(...)
            {
                log(
                    "[LB] Unknown client error"
                );


                if(client != INVALID_SOCKET)
                {
                    shutdown(
                        client,
                        SD_BOTH
                    );

                    closesocket(client);
                }
            }
        }
    }



// ------------------------------------------------------------
// START WORKER THREADS
// ------------------------------------------------------------

void LoadBalancer::startWorkers()
    {
        for(
            int i = 0;
            i < WORKER_THREADS;
            i++
        )
        {
            workerThreads.emplace_back(
                &LoadBalancer::worker,
                this
            );
        }


        log(
            "[LB] Worker threads started: " +
            to_string(WORKER_THREADS)
        );
    }

