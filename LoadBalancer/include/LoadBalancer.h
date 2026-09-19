#pragma once

#include "Common.h"
#include "BackendServer.h"

// ============================================================
// LOAD BALANCER
//
// Accepts client connections, picks the healthiest backend
// (least-connections), and relays raw TCP traffic between the
// client and that backend.
//
// Method bodies live in:
//   - LoadBalancer.cpp        (construction, health checks, helpers)
//   - LoadBalancer_Relay.cpp  (relay / handleClient / worker threads)
//   - LoadBalancer_Init.cpp   (socket setup + main accept loop)
// ============================================================

class LoadBalancer
{
private:

    SOCKET listenSocket = INVALID_SOCKET;

    /*
        BackendServer contains atomic variables.

        Therefore we use unique_ptr so that the actual
        BackendServer objects never need to be copied/moved.
    */
    vector<unique_ptr<BackendServer>> backends;

    atomic<bool> running{true};

    mutex logMutex;


    // ========================================================
    // CLIENT QUEUE
    // ========================================================

    queue<SOCKET> clientQueue;

    mutex queueMutex;

    condition_variable queueCV;

    vector<thread> workerThreads;


public:

    // ========================================================
    // CONSTRUCTOR / DESTRUCTOR
    // ========================================================

    LoadBalancer();

    ~LoadBalancer();


    // ========================================================
    // LOG
    // ========================================================

    void log(const string& message);


    // ========================================================
    // SOCKET HELPERS
    // ========================================================

    void configureSocket(SOCKET sock);

    SOCKET connectToBackend(
        BackendServer& backend
    );


    // ========================================================
    // HEALTH CHECKING
    // ========================================================

    bool checkBackend(
        BackendServer& backend
    );

    void healthChecker();

    vector<int> getBackendOrder();


    // ========================================================
    // I/O HELPERS
    // ========================================================

    bool sendAll(
        SOCKET sock,
        const char* data,
        int length
    );

    void send503(
        SOCKET client
    );

    void relay(
        SOCKET client,
        SOCKET backend
    );


    // ========================================================
    // CLIENT HANDLING
    // ========================================================

    void handleClient(
        SOCKET client
    );

    void worker();

    void startWorkers();


    // ========================================================
    // STARTUP / MAIN LOOP
    // ========================================================

    bool initialize();

    void start();
};
