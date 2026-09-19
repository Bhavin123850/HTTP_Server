#pragma once

#include "Common.h"
#include "Config.h"
#include "ConnectionPool.h"
#include "DataStore.h"
#include "LRUCache.h"
#include "TableCache.h"
#include "TrieRouter.h"
#include "ThreadPool.h"
#include "HTTPRequest.h"

// ============================================================
// HTTP SERVER
//
// Ties everything together: accepts connections, routes them
// via the Trie router, and serves customers/products/orders
// through DataStore + caches.
//
// Method bodies live in the .cpp files under src/:
//   - HTTPServer.cpp            (setup, parsing, plumbing)
//   - HTTPServer_Customers.cpp  (handleCustomers)
//   - HTTPServer_Products.cpp   (handleProducts)
//   - HTTPServer_Orders.cpp     (handleOrders)
// ============================================================

class HTTPServer
{
private:

    SOCKET serverSocket;

    int PORT;


    // --------------------------------------------------------
    // THREAD POOL
    // --------------------------------------------------------

    ThreadPool pool;


    // --------------------------------------------------------
    // TRIE ROUTER
    // --------------------------------------------------------

    TrieRouter router;


    // --------------------------------------------------------
    // DATABASE
    // --------------------------------------------------------

    ConnectionPool databasePool;

    DataStore dataStore;


    // --------------------------------------------------------
    // KEY-VALUE CACHES
    // --------------------------------------------------------

    LRUCache customerCache;

    LRUCache productCache;

    LRUCache orderCache;


    // --------------------------------------------------------
    // TABLE CACHES
    // --------------------------------------------------------

    TableCache customerTableCache;

    TableCache productTableCache;

    TableCache orderTableCache;


public:

    // ========================================================
    // CONSTRUCTOR / DESTRUCTOR
    // ========================================================

    HTTPServer(
        int port,
        int numberOfThreads
    );

    ~HTTPServer();


    // ========================================================
    // ROUTING / SETUP
    // ========================================================

    void registerRoutes();


    // ========================================================
    // REQUEST HELPERS
    // ========================================================

    string getPath(
        const string& fullPath
    );

    int getIdFromQuery(
        const string& fullPath
    );

    string receiveRequest(
        SOCKET clientSocket
    );

    HTTPRequest parseRequest(
        const string& rawRequest
    );

    string createResponse(
        int statusCode,
        const string& statusText,
        const string& body,
        const string& contentType = "application/json"
    );

    void sendResponse(
        SOCKET clientSocket,
        const string& response
    );


    // ========================================================
    // REQUEST HANDLING
    // ========================================================

    void handleClient(
        SOCKET clientSocket
    );

    void handleCustomers(
        const HTTPRequest& request,
        SOCKET clientSocket
    );

    void handleProducts(
        const HTTPRequest& request,
        SOCKET clientSocket
    );

    void handleOrders(
        const HTTPRequest& request,
        SOCKET clientSocket
    );


    // ========================================================
    // MAIN LOOP
    // ========================================================

    void start();
};