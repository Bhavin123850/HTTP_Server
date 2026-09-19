# HTTP Server Project — Split Into Smaller Files

Your original 4 files (~250 KB total) have been broken down into small,
single-responsibility files, grouped by folder. No logic was changed —
only reorganized. Every class keeps its original code; class *declarations*
now live in `.h` files and method *bodies* live in `.cpp` files.

## Why this structure

`server1.cpp`, `server2.cpp`, and `server3.cpp` were **identical** except
for one number (the port: 8080 / 8000 / 9000). So instead of 3 giant
copies of the same 3,700-line file, there is now:

- **One shared codebase** (`Server/`) containing all the actual logic.
- **Three tiny folders** (`Server1/`, `Server2/`, `Server3/`) that each
  contain a 15-line `main.cpp` picking a port and starting the shared
  server.

If you ever need to fix a bug or add a feature, you now do it **once**
in `Server/`, and all three servers get the fix — instead of editing
three 72 KB files by hand.

## Folder layout

```
HttpServerProject/
├── LoadBalancer/
│   ├── include/
│   │   ├── Common.h          # includes + constants (port, timeouts, buffer size...)
│   │   ├── BackendServer.h   # struct describing one backend (host/port/health)
│   │   └── LoadBalancer.h    # class declaration (all method signatures)
│   ├── src/
│   │   ├── LoadBalancer.cpp        # construction, logging, socket config, health checks
│   │   ├── LoadBalancer_Relay.cpp  # TCP relay loop, per-client handling, worker threads
│   │   └── LoadBalancer_Init.cpp   # winsock/bind/listen setup + main accept loop
│   └── main/
│       └── main.cpp          # int main() { LoadBalancer lb; lb.start(); }
│
├── Server/                    # <-- shared by all 3 backend servers
│   ├── include/
│   │   ├── Common.h           # includes shared by every server file
│   │   ├── Config.h           # DB_CONNECTION string
│   │   ├── ConnectionPool.h   # PostgreSQL connection pool
│   │   ├── DataStore.h        # SQL access declarations (customers/products/orders)
│   │   ├── LRUCache.h         # key -> value cache (used for GET ?id=)
│   │   ├── TableCache.h       # whole-table cache (used for GET /customers etc.)
│   │   ├── TrieRouter.h       # trie-based URL router
│   │   ├── ThreadPool.h       # task queue + worker thread pool
│   │   ├── HTTPRequest.h      # simple {method, path, body} struct
│   │   └── HTTPServer.h       # HTTP server class declaration
│   └── src/
│       ├── DataStore_Customers.cpp   # SQL for /customers
│       ├── DataStore_Products.cpp    # SQL for /products
│       ├── DataStore_Orders.cpp      # SQL for /orders
│       ├── HTTPServer.cpp            # construction, routing, send/receive, accept loop
│       ├── HTTPServer_Customers.cpp  # GET/POST/PUT handling for /customers
│       ├── HTTPServer_Products.cpp   # GET/POST/PUT handling for /products
│       └── HTTPServer_Orders.cpp     # GET/POST/PUT handling for /orders
│
├── Server1/
│   └── main.cpp    # starts HTTPServer on port 8080
├── Server2/
│   └── main.cpp    # starts HTTPServer on port 8000
└── Server3/
    └── main.cpp    # starts HTTPServer on port 9000
```

Every file is now well under 300 lines (most are 100–250), instead of one
1,700–3,700 line file.

## How to build (Visual Studio / MSVC, Windows)

You'll need the same dependencies your original code needed:
- **libpqxx** (PostgreSQL C++ client) — for the servers only, not the load balancer
- **nlohmann/json** — for the servers only
- **ws2_32.lib** (winsock) — already linked via `#pragma comment` in the code

Recommended setup — create **4 separate projects** in one Visual Studio solution:

1. **LoadBalancer** project
   - Add all files under `LoadBalancer/include` and `LoadBalancer/src` and `LoadBalancer/main`
   - Add `LoadBalancer/include` to "Additional Include Directories"
   - No third-party dependencies needed (just Winsock)

2. **Server1**, **Server2**, **Server3** projects
   - Each one adds:
     - all files under `Server/include` and `Server/src` (shared)
     - its own single `main.cpp` (e.g. `Server1/main.cpp`)
   - Add `Server/include` to "Additional Include Directories" for each project
   - Link libpqxx + nlohmann/json (via vcpkg is easiest: `vcpkg install libpqxx nlohmann-json`)

If you use vcpkg with manifest mode or a solution-wide vcpkg integration,
all three server projects can share the same installed packages.

## What did NOT change

- Every function's logic, every SQL query, every socket call is byte-for-byte
  the same code as your original files — just moved into a different file
  and given a `ClassName::` prefix where it became an out-of-line definition.
- Server1/2/3 still listen on 8080/8000/9000 respectively, matching what the
  LoadBalancer already expects (see `LoadBalancer/include/LoadBalancer.h`
  constructor, which points at `127.0.0.1:8080`, `:8000`, `:9000`).
