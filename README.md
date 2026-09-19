# HTTP Server + Load Balancer (C++ / Windows / PostgreSQL)

A multi-threaded HTTP server cluster written in C++ for Windows, fronted by a
custom TCP load balancer. Three identical backend server instances handle
`customers`, `products`, and `orders` resources backed by PostgreSQL, with
in-memory caching and a Trie-based router. The load balancer distributes
incoming connections across the healthiest backend using a least-connections
strategy, with automatic health checks and failover.

## Features

- **Custom TCP load balancer**
  - Least-connections routing across backend servers
  - Background health checker with automatic failover
  - Worker-thread pool (no thread-per-connection)
  - Raw socket relay between client and backend
- **HTTP server (x3 instances)**
  - Hand-rolled HTTP parsing over raw sockets (Winsock)
  - Trie-based URL router
  - Thread pool for concurrent request handling
  - PostgreSQL access via `libpqxx`, pooled connections
  - Two-tier caching: per-record LRU cache + whole-table cache
  - JSON responses via `nlohmann/json`
  - REST endpoints for `/customers`, `/products`, `/orders` (GET / POST / PUT)

## Architecture

```
                        ┌─────────────────┐
        clients ──────► │   LoadBalancer   │  (port 7000)
                        │ least-connections │
                        └───────┬──────────┘
                    ┌───────────┼───────────┐
                    ▼           ▼           ▼
              ┌─────────┐ ┌─────────┐ ┌─────────┐
              │ Server1 │ │ Server2 │ │ Server3 │
              │  :8080  │ │  :8000  │ │  :9000  │
              └────┬────┘ └────┬────┘ └────┬────┘
                   └────────────┼───────────┘
                                ▼
                          PostgreSQL
                         (HttpServerDB)
```

Server1, Server2, and Server3 run the **exact same code** — they only differ
by which port they listen on. That shared logic lives once in `Server/` and
each server folder just wires up a `main()` with its own port.

## Project structure

```
HttpServerProject/
├── LoadBalancer/
│   ├── include/
│   │   ├── Common.h          # includes + tunables (port, timeouts, buffer size, worker count)
│   │   ├── BackendServer.h   # struct describing one backend (host/port/health/active connections)
│   │   └── LoadBalancer.h    # LoadBalancer class declaration
│   ├── src/
│   │   ├── LoadBalancer.cpp        # construction, logging, socket config, health checks
│   │   ├── LoadBalancer_Relay.cpp  # TCP relay loop, per-client handling, worker threads
│   │   └── LoadBalancer_Init.cpp   # winsock/bind/listen setup + main accept loop
│   └── main/
│       └── main.cpp          # entry point
│
├── Server/                    # shared by all 3 backend servers
│   ├── include/
│   │   ├── Common.h           # includes shared by every server file
│   │   ├── Config.h           # PostgreSQL connection string
│   │   ├── ConnectionPool.h   # pooled libpqxx connections
│   │   ├── DataStore.h        # SQL access declarations (customers/products/orders)
│   │   ├── LRUCache.h         # per-record cache (GET ?id=)
│   │   ├── TableCache.h       # whole-table cache (GET /customers, etc.)
│   │   ├── TrieRouter.h       # trie-based URL router
│   │   ├── ThreadPool.h       # task queue + worker thread pool
│   │   ├── HTTPRequest.h      # {method, path, body} struct
│   │   └── HTTPServer.h       # HTTP server class declaration
│   └── src/
│       ├── DataStore_Customers.cpp
│       ├── DataStore_Products.cpp
│       ├── DataStore_Orders.cpp
│       ├── HTTPServer.cpp            # construction, routing, send/receive, accept loop
│       ├── HTTPServer_Customers.cpp  # GET/POST/PUT for /customers
│       ├── HTTPServer_Products.cpp   # GET/POST/PUT for /products
│       └── HTTPServer_Orders.cpp     # GET/POST/PUT for /orders
│
├── Server1/main.cpp    # starts HTTPServer on port 8080
├── Server2/main.cpp    # starts HTTPServer on port 8000
└── Server3/main.cpp    # starts HTTPServer on port 9000
```

## Requirements

- Windows + Visual Studio (MSVC) — the code uses `winsock2.h` / `ws2tcpip.h`
- PostgreSQL (local instance is fine)
- [vcpkg](https://github.com/microsoft/vcpkg) for dependencies:
  - `libpqxx` — PostgreSQL C++ client
  - `nlohmann-json` — JSON library

The `LoadBalancer` target has **no third-party dependencies** (Winsock only).

## Setup

### 1. Install dependencies

```powershell
vcpkg install libpqxx nlohmann-json
vcpkg integrate install
```

### 2. Configure the database

Create a PostgreSQL database and update the connection string in
[`Server/include/Config.h`](Server/include/Config.h):

```cpp
const string DB_CONNECTION =
    "host=127.0.0.1 "
    "port=5432 "
    "dbname=HttpServerDB "
    "user=postgres "
    "password=YOUR_PASSWORD_HERE";
```

> ⚠️ Don't commit real credentials. See [Configuration](#configuration) below
> for a safer approach using environment variables.

Create the `customers`, `products`, and `orders` tables matching the schema
used in `Server/src/DataStore_*.cpp`.

### 3. Build

Open the solution in Visual Studio with **4 projects**:

| Project      | Sources                                                             | Include dirs                     | Dependencies              |
|--------------|----------------------------------------------------------------------|-----------------------------------|----------------------------|
| LoadBalancer | `LoadBalancer/include`, `LoadBalancer/src`, `LoadBalancer/main`      | `LoadBalancer/include`           | none                       |
| Server1      | `Server/include`, `Server/src`, `Server1/main.cpp`                   | `Server/include`                 | libpqxx, nlohmann-json     |
| Server2      | `Server/include`, `Server/src`, `Server2/main.cpp`                   | `Server/include`                 | libpqxx, nlohmann-json     |
| Server3      | `Server/include`, `Server/src`, `Server3/main.cpp`                   | `Server/include`                 | libpqxx, nlohmann-json     |

Build the solution (Release or Debug).

### 4. Run

Start the three backend servers first, then the load balancer:

```powershell
Server1.exe    # listens on :8080
Server2.exe    # listens on :8000
Server3.exe    # listens on :9000
LoadBalancer.exe   # listens on :7000, routes to the servers above
```

### 5. Test

Send requests through the load balancer (not directly to a server):

```bash
curl http://127.0.0.1:7000/customers
curl http://127.0.0.1:7000/products?id=1
curl -X POST http://127.0.0.1:7000/orders -d '{"customerId":1,"productId":2,"quantity":3}'
```

## Configuration

| Setting              | Location                                  | Default        |
|----------------------|--------------------------------------------|----------------|
| Load balancer port   | `LoadBalancer/include/Common.h`            | `7000`         |
| Backend ports        | `Server1/2/3/main.cpp`                     | `8080/8000/9000` |
| Worker thread count  | `LoadBalancer/include/Common.h`            | `128`          |
| DB connection string | `Server/include/Config.h`                  | see above      |

### Avoiding hardcoded DB credentials

`Config.h` currently hardcodes the database password. For anything beyond
local testing, prefer reading it from an environment variable instead, e.g.:

```cpp
const string DB_CONNECTION =
    "host=127.0.0.1 port=5432 dbname=HttpServerDB "
    "user=postgres password=" + string(getenv("DB_PASSWORD"));
```

and add `Config.h` (or just the credentials) to `.gitignore` if you go this route.

## License

Add your preferred license here (MIT, Apache-2.0, etc.).
