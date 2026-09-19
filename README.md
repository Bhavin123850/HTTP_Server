# HTTP Server with Load Balancer

A simple project built in C++ that shows how a **load balancer** distributes
traffic across multiple **backend servers**, with data stored in
**PostgreSQL**.

## What this project does

- 3 backend servers run at the same time, each handling HTTP requests for
  `customers`, `products`, and `orders`.
- 1 load balancer sits in front of them. It receives every request first,
  then forwards it to whichever server is least busy.
- If a server goes down, the load balancer notices (via health checks) and
  stops sending it traffic until it recovers.
- Each server talks to a PostgreSQL database and uses caching to respond
  faster.

## How it works (simple flow)

```
You / Client
     |
     v
Load Balancer (port 7000)
     |
     +--> Server 1 (port 8080)
     +--> Server 2 (port 8000)   --->  PostgreSQL Database
     +--> Server 3 (port 9000)
```

The load balancer picks whichever server has the fewest active connections,
so traffic is spread out evenly.

## Project folders

- **LoadBalancer/** — the load balancer code
- **Server/** — the shared code used by all 3 servers (routes, database
  access, caching)
- **Server1/**, **Server2/**, **Server3/** — each just starts a server on a
  different port using the shared code above

## What you need to run it

- Windows + Visual Studio
- PostgreSQL installed
- Two libraries installed via vcpkg: `libpqxx` and `nlohmann-json`

## How to run it

1. Create a PostgreSQL database and update the username/password in
   `Server/include/Config.h`.
2. Build 4 projects in Visual Studio: `LoadBalancer`, `Server1`, `Server2`,
   `Server3`.
3. Start the servers first:
   ```
   Server1.exe
   Server2.exe
   Server3.exe
   ```
4. Then start the load balancer:
   ```
   LoadBalancer.exe
   ```
5. Send requests to the load balancer, not the servers directly:
   ```
   http://127.0.0.1:7000/customers
   http://127.0.0.1:7000/products
   http://127.0.0.1:7000/orders
   ```

That's it — the load balancer will route each request to one of the 3
servers automatically.
