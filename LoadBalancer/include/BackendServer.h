#pragma once

#include "Common.h"

// ============================================================
// BACKEND SERVER
// ============================================================

struct BackendServer
{
    string host;
    int port;

    atomic<int> activeConnections{0};
    atomic<bool> healthy{true};

    BackendServer(
        const string& host,
        int port
    )
        : host(host),
          port(port)
    {
    }

    BackendServer(const BackendServer&) = delete;

    BackendServer& operator=(
        const BackendServer&
    ) = delete;
};
