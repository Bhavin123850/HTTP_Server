#pragma once

// ============================================================
// COMMON INCLUDES FOR THE LOAD BALANCER
// ============================================================

#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <chrono>
#include <string>
#include <cstring>
#include <limits>
#include <queue>
#include <condition_variable>

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;


// ============================================================
// CONFIGURATION
// ============================================================

constexpr int LOAD_BALANCER_PORT = 7000;

constexpr int BACKLOG = 1024;

constexpr int CONNECT_TIMEOUT_MS = 3000;

constexpr int SOCKET_TIMEOUT_MS = 30000;

constexpr int HEALTH_CHECK_INTERVAL_MS = 2000;

constexpr int BUFFER_SIZE = 64 * 1024;

// Number of worker threads.
// Do NOT create one thread per client.
constexpr int WORKER_THREADS = 128;
