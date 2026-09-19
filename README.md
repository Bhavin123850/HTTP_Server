# HTTP Server Project

A C++ HTTP server project with a load balancer, three backend servers, and a
PostgreSQL database.

**Features:**

- 3 backend HTTP servers + 1 load balancer
- PostgreSQL database access
- Thread pool
- Connection pool
- LRU cache
- Table cache
- Trie router
- TCP socket communication
- Backend health checking

---

## Project Structure

```text
HttpServerProject/
│
├── Server/
│   ├── include/
│   │   ├── Common.h
│   │   ├── Config.h
│   │   ├── ConnectionPool.h
│   │   ├── DataStore.h
│   │   ├── HTTPRequest.h
│   │   ├── HTTPServer.h
│   │   ├── LRUCache.h
│   │   ├── TableCache.h
│   │   ├── ThreadPool.h
│   │   └── TrieRouter.h
│   │
│   └── src/
│       ├── DataStore_Customers.cpp
│       ├── DataStore_Orders.cpp
│       ├── DataStore_Products.cpp
│       ├── HTTPServer.cpp
│       ├── HTTPServer_Customers.cpp
│       ├── HTTPServer_Orders.cpp
│       └── HTTPServer_Products.cpp
│
├── Server1/
│   └── main.cpp
│
├── Server2/
│   └── main.cpp
│
├── Server3/
│   └── main.cpp
│
├── LoadBalancer/
│   ├── include/
│   │   ├── BackendServer.h
│   │   ├── Common.h
│   │   └── LoadBalancer.h
│   │
│   ├── src/
│   │   ├── LoadBalancer.cpp
│   │   ├── LoadBalancer_Init.cpp
│   │   └── LoadBalancer_Relay.cpp
│   │
│   └── main/
│       └── main.cpp
│
└── README.md
```

---

## Architecture

```text
                    Client
                       |
                  Port 7000
                       |
                 Load Balancer
                 /      |      \
                /       |       \
               ↓        ↓        ↓
        Server 1    Server 2    Server 3
        Port 8080   Port 8000   Port 9000
               \        |        /
                \       |       /
                 PostgreSQL
                   Port 5432
```

## Ports

| Component      | Port |
|-----------------|------|
| Load Balancer   | 7000 |
| Server 1        | 8080 |
| Server 2        | 8000 |
| Server 3        | 9000 |
| PostgreSQL      | 5432 |

---

## Requirements

Install the following:

- MSYS2 UCRT64
- GCC / G++
- PostgreSQL
- libpqxx
- nlohmann/json

The project uses C++, Winsock, libpqxx, PostgreSQL, and nlohmann/json.

## Database

The servers connect to PostgreSQL using:

```text
Host:     127.0.0.1
Port:     5432
Database: HttpServerDB
User:     postgres
```

Make sure PostgreSQL is running, and that the `HttpServerDB` database exists
with the required tables, before starting the servers.

---

## Build and Run

Open **4 separate MSYS2 UCRT64 terminals** — one per component.

### Terminal 1 — Server 1

```bash
cd "C:\Users\ASUS\Downloads\HttpServerProject\HttpServerProject\Server"

g++ -Iinclude src/DataStore_Customers.cpp src/DataStore_Products.cpp src/DataStore_Orders.cpp src/HTTPServer.cpp src/HTTPServer_Customers.cpp src/HTTPServer_Products.cpp src/HTTPServer_Orders.cpp ../Server1/main.cpp -lpqxx -lpq -lws2_32 -o Server1.exe

./Server1.exe
```

Runs on `http://127.0.0.1:8080`

### Terminal 2 — Server 2

```bash
cd "C:\Users\ASUS\Downloads\HttpServerProject\HttpServerProject\Server"

g++ -Iinclude src/DataStore_Customers.cpp src/DataStore_Products.cpp src/DataStore_Orders.cpp src/HTTPServer.cpp src/HTTPServer_Customers.cpp src/HTTPServer_Products.cpp src/HTTPServer_Orders.cpp ../Server2/main.cpp -lpqxx -lpq -lws2_32 -o Server2.exe

./Server2.exe
```

Runs on `http://127.0.0.1:8000`

### Terminal 3 — Server 3

```bash
cd "C:\Users\ASUS\Downloads\HttpServerProject\HttpServerProject\Server"

g++ -Iinclude src/DataStore_Customers.cpp src/DataStore_Products.cpp src/DataStore_Orders.cpp src/HTTPServer.cpp src/HTTPServer_Customers.cpp src/HTTPServer_Products.cpp src/HTTPServer_Orders.cpp ../Server3/main.cpp -lpqxx -lpq -lws2_32 -o Server3.exe

./Server3.exe
```

Runs on `http://127.0.0.1:9000`

### Terminal 4 — Load Balancer

```bash
cd "C:\Users\ASUS\Downloads\HttpServerProject\HttpServerProject\LoadBalancer"

g++ -Iinclude src/LoadBalancer.cpp src/LoadBalancer_Init.cpp src/LoadBalancer_Relay.cpp main/main.cpp -lws2_32 -o LoadBalancer.exe

./LoadBalancer.exe
```

Runs on `http://127.0.0.1:7000`

---

## Start Order

Always start components in this order:

1. PostgreSQL
2. Server 1
3. Server 2
4. Server 3
5. Load Balancer

The load balancer health-checks the backend servers, so all three servers
must already be running before you start it.

---

## How the Load Balancer Works

The load balancer listens on port `7000`. When a client sends a request:

```text
Client
   |
   ↓
Load Balancer :7000
   |
   ├── Server 1 :8080
   ├── Server 2 :8000
   └── Server 3 :9000
```

It checks the health of each backend server, picks an available one, and
forwards the request there using a pool of worker threads.

---

## Server Features

**HTTP Server** — provides endpoints for Customers, Products, and Orders.
All three servers run the same code; only the port differs (8080 / 8000 / 9000).

**Thread Pool** — handles multiple client requests concurrently.

**Connection Pool** — reuses a fixed set of database connections instead of
opening a new one per request.

**LRU Cache** — caches frequently requested individual records.

**Table Cache** — caches whole-table results to reduce database queries.

**Trie Router** — matches incoming HTTP routes using a Trie structure.

**Load Balancer** — accepts client connections, checks backend health,
selects a backend server, forwards requests, relays responses, and uses
worker threads to handle everything concurrently.
