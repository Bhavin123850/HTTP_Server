#pragma once

// ============================================================
// COMMON STANDARD LIBRARY INCLUDES
// Shared by every module of the HTTP server.
// ============================================================

#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>
#include <list>
#include <memory>
#include <stdexcept>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <pqxx/pqxx>
#include <nlohmann/json.hpp>

#pragma comment(lib, "ws2_32.lib")

using namespace std;
using json = nlohmann::json;
