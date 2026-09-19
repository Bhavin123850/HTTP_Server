#include "HTTPServer.h"

// ============================================================
// HTTPServer: construction, teardown, routing table, request
// parsing/response plumbing, and the accept loop.
// ============================================================

// ------------------------------------------------------------
// CONSTRUCTOR
// ------------------------------------------------------------

HTTPServer::HTTPServer(
        int port,
        int numberOfThreads
    )
        : serverSocket(INVALID_SOCKET),
          PORT(port),
          pool(numberOfThreads),
          databasePool(
              DB_CONNECTION,
              15
          ),
          dataStore(databasePool),
          customerCache(100),
          productCache(100),
          orderCache(100)
    {
        // ----------------------------------------------------
        // START WINSOCK
        // ----------------------------------------------------

        WSADATA wsaData;


        int result =
            WSAStartup(
                MAKEWORD(2, 2),
                &wsaData
            );


        if(result != 0)
        {
            cout
                << "WSAStartup failed."
                << endl;


            exit(1);
        }


        // ----------------------------------------------------
        // CREATE SOCKET
        // ----------------------------------------------------

        serverSocket =
            socket(
                AF_INET,
                SOCK_STREAM,
                IPPROTO_TCP
            );


        if(
            serverSocket
            ==
            INVALID_SOCKET
        )
        {
            cout
                << "Socket creation failed."
                << endl;


            WSACleanup();


            exit(1);
        }


        // ----------------------------------------------------
        // REUSE ADDRESS
        // ----------------------------------------------------

        int opt = 1;


        setsockopt(
            serverSocket,
            SOL_SOCKET,
            SO_REUSEADDR,
            (char*)&opt,
            sizeof(opt)
        );


        // ----------------------------------------------------
        // SERVER ADDRESS
        // ----------------------------------------------------

        sockaddr_in serverAddress{};


        serverAddress.sin_family =
            AF_INET;


        serverAddress.sin_addr.s_addr =
            INADDR_ANY;


        serverAddress.sin_port =
            htons(PORT);


        // ----------------------------------------------------
        // BIND
        // ----------------------------------------------------

        result =
            bind(
                serverSocket,
                (sockaddr*)&serverAddress,
                sizeof(serverAddress)
            );


        if(
            result
            ==
            SOCKET_ERROR
        )
        {
            cout
                << "Bind failed."
                << endl;


            closesocket(
                serverSocket
            );


            WSACleanup();


            exit(1);
        }


        // ----------------------------------------------------
        // LISTEN
        // ----------------------------------------------------

        result =
            listen(
                serverSocket,
                SOMAXCONN
            );


        if(
            result
            ==
            SOCKET_ERROR
        )
        {
            cout
                << "Listen failed."
                << endl;


            closesocket(
                serverSocket
            );


            WSACleanup();


            exit(1);
        }


        // ----------------------------------------------------
        // REGISTER ROUTES
        // ----------------------------------------------------

        registerRoutes();


        cout << endl;


        cout
            << "========================================"
            << endl;


        cout
            << "       C++ E-COMMERCE SERVER"
            << endl;


        cout
            << "========================================"
            << endl;


        cout
            << "Port: "
            << PORT
            << endl;


        cout
            << "Worker Threads: "
            << numberOfThreads
            << endl;


        cout
            << "Database Connections: 10"
            << endl;


        cout
            << "Key-Value LRU Cache: ENABLED"
            << endl;


        cout
            << "Table Cache: ENABLED"
            << endl;


        cout
            << "========================================"
            << endl;
    }



// ------------------------------------------------------------
// DESTRUCTOR
// ------------------------------------------------------------

HTTPServer::~HTTPServer()
    {
        closesocket(
            serverSocket
        );


        WSACleanup();
    }



// ------------------------------------------------------------
// REGISTER ROUTES
// ------------------------------------------------------------

void HTTPServer::registerRoutes()
    {
        router.insert(
            "/",
            "home"
        );


        router.insert(
            "/customers",
            "customers"
        );


        router.insert(
            "/products",
            "products"
        );


        router.insert(
            "/orders",
            "orders"
        );
    }



// ------------------------------------------------------------
// GET PATH WITHOUT QUERY STRING
// ------------------------------------------------------------

string HTTPServer::getPath(
        const string& fullPath
    )
    {
        size_t position =
            fullPath.find('?');


        if(
            position
            ==
            string::npos
        )
        {
            return fullPath;
        }


        return fullPath.substr(
            0,
            position
        );
    }



// ------------------------------------------------------------
// GET ID FROM QUERY STRING
// ------------------------------------------------------------

int HTTPServer::getIdFromQuery(
        const string& fullPath
    )
    {
        size_t position =
            fullPath.find("id=");


        if(
            position
            ==
            string::npos
        )
        {
            return -1;
        }


        string value =
            fullPath.substr(
                position + 3
            );


        try
        {
            return stoi(value);
        }
        catch(...)
        {
            return -1;
        }
    }



// ------------------------------------------------------------
// RECEIVE REQUEST
// ------------------------------------------------------------

string HTTPServer::receiveRequest(
        SOCKET clientSocket
    )
    {
        string request;


        char buffer[4096];


        while(true)
        {
            memset(
                buffer,
                0,
                sizeof(buffer)
            );


            int bytes =
                recv(
                    clientSocket,
                    buffer,
                    sizeof(buffer),
                    0
                );


            if(bytes <= 0)
            {
                break;
            }


            request.append(
                buffer,
                bytes
            );


            size_t headerEnd =
                request.find(
                    "\r\n\r\n"
                );


            if(
                headerEnd
                ==
                string::npos
            )
            {
                continue;
            }


            size_t contentPosition =
                request.find(
                    "Content-Length:"
                );


            if(
                contentPosition
                ==
                string::npos
            )
            {
                break;
            }


            size_t lineEnd =
                request.find(
                    "\r\n",
                    contentPosition
                );


            string lengthLine =
                request.substr(
                    contentPosition,
                    lineEnd - contentPosition
                );


            size_t colon =
                lengthLine.find(':');


            int contentLength = 0;


            if(
                colon
                !=
                string::npos
            )
            {
                contentLength =
                    stoi(
                        lengthLine.substr(
                            colon + 1
                        )
                    );
            }


            size_t bodyStart =
                headerEnd + 4;


            size_t bodyReceived =
                request.size()
                -
                bodyStart;


            if(
                bodyReceived
                >=
                static_cast<size_t>(
                    contentLength
                )
            )
            {
                break;
            }
        }


        return request;
    }



// ------------------------------------------------------------
// PARSE REQUEST
// ------------------------------------------------------------

HTTPRequest HTTPServer::parseRequest(
        const string& request
    )
    {
        HTTPRequest httpRequest;


        size_t headerEnd =
            request.find(
                "\r\n\r\n"
            );


        string headers;


        if(
            headerEnd
            !=
            string::npos
        )
        {
            headers =
                request.substr(
                    0,
                    headerEnd
                );


            httpRequest.body =
                request.substr(
                    headerEnd + 4
                );
        }
        else
        {
            headers =
                request;
        }


        stringstream stream(
            headers
        );


        string firstLine;


        getline(
            stream,
            firstLine
        );


        string version;


        stringstream firstLineStream(
            firstLine
        );


        firstLineStream
            >> httpRequest.method
            >> httpRequest.path
            >> version;


        return httpRequest;
    }



// ------------------------------------------------------------
// CREATE HTTP RESPONSE
// ------------------------------------------------------------

string HTTPServer::createResponse(
        int statusCode,
        const string& statusText,
        const string& body,
        const string& contentType
    )
    {
        stringstream response;


        response
            << "HTTP/1.1 "
            << statusCode
            << " "
            << statusText
            << "\r\n";


        response
            << "Content-Type: "
            << contentType
            << "\r\n";


        response
            << "Content-Length: "
            << body.size()
            << "\r\n";


        response
            << "Connection: close"
            << "\r\n";


        response
            << "\r\n";


        response
            << body;


        return response.str();
    }



// ------------------------------------------------------------
// SEND RESPONSE
// ------------------------------------------------------------

void HTTPServer::sendResponse(
        SOCKET clientSocket,
        const string& response
    )
    {
        size_t totalSent = 0;


        while(
            totalSent
            <
            response.size()
        )
        {
            int sent =
                send(
                    clientSocket,
                    response.c_str()
                    +
                    totalSent,
                    static_cast<int>(
                        response.size()
                        -
                        totalSent
                    ),
                    0
                );


            if(sent <= 0)
            {
                break;
            }


            totalSent += sent;
        }
    }



// ------------------------------------------------------------
// HANDLE CLIENT (routes to Customers/Products/Orders handlers)
// ------------------------------------------------------------

void HTTPServer::handleClient(
        SOCKET clientSocket
    )
    {
        try
        {
            // ------------------------------------------------
            // RECEIVE
            // ------------------------------------------------

            string rawRequest =
                receiveRequest(
                    clientSocket
                );


            if(rawRequest.empty())
            {
                closesocket(
                    clientSocket
                );


                return;
            }


            // ------------------------------------------------
            // PARSE
            // ------------------------------------------------

            HTTPRequest request =
                parseRequest(
                    rawRequest
                );


            cout
                << "\n[Thread "
                << this_thread::get_id()
                << "] "
                << request.method
                << " "
                << request.path
                << endl;


            // ------------------------------------------------
            // GET PATH
            // ------------------------------------------------

            string path =
                getPath(
                    request.path
                );


            // ------------------------------------------------
            // TRIE ROUTING
            // ------------------------------------------------

            string endpoint =
                router.search(
                    path
                );


            if(endpoint.empty())
            {
                string body =
                    R"({"error":"Endpoint not found"})";


                sendResponse(
                    clientSocket,
                    createResponse(
                        404,
                        "Not Found",
                        body
                    )
                );


                closesocket(
                    clientSocket
                );


                return;
            }


            // ------------------------------------------------
            // ROUTE
            // ------------------------------------------------

            if(
                endpoint
                ==
                "customers"
            )
            {
                handleCustomers(
                    request,
                    clientSocket
                );
            }
            else if(
                endpoint
                ==
                "products"
            )
            {
                handleProducts(
                    request,
                    clientSocket
                );
            }
            else if(
                endpoint
                ==
                "orders"
            )
            {
                handleOrders(
                    request,
                    clientSocket
                );
            }
            else if(
                endpoint
                ==
                "home"
            )
            {
                json body;


                body["server"] =
                    "C++ E-Commerce Server";


                body["database"] =
                    "PostgreSQL";


                body["key_value_cache"] =
                    "LRU";


                body["table_cache"] =
                    "Enabled";


                body["database_pool"] =
                    "Enabled";


                body["database_connections"] =
                    10;


                body["status"] =
                    "running";


                sendResponse(
                    clientSocket,
                    createResponse(
                        200,
                        "OK",
                        body.dump(4)
                    )
                );
            }
        }


        // ====================================================
        // DUPLICATE
        // ====================================================

        catch(
            const pqxx::unique_violation& e
        )
        {
            string body =
                json{
                    {
                        "error",
                        "Duplicate value"
                    },
                    {
                        "details",
                        e.what()
                    }
                }.dump(4);


            sendResponse(
                clientSocket,
                createResponse(
                    409,
                    "Conflict",
                    body
                )
            );
        }


        // ====================================================
        // FOREIGN KEY
        // ====================================================

        catch(
            const pqxx::foreign_key_violation& e
        )
        {
            string body =
                json{
                    {
                        "error",
                        "Invalid foreign key"
                    },
                    {
                        "details",
                        e.what()
                    }
                }.dump(4);


            sendResponse(
                clientSocket,
                createResponse(
                    400,
                    "Bad Request",
                    body
                )
            );
        }


        // ====================================================
        // DATABASE ERROR
        // ====================================================

        catch(
            const pqxx::sql_error& e
        )
        {
            string body =
                json{
                    {
                        "error",
                        "Database error"
                    },
                    {
                        "details",
                        e.what()
                    }
                }.dump(4);


            sendResponse(
                clientSocket,
                createResponse(
                    500,
                    "Internal Server Error",
                    body
                )
            );
        }


        // ====================================================
        // GENERAL ERROR
        // ====================================================

        catch(
            const exception& e
        )
        {
            string body =
                json{
                    {
                        "error",
                        e.what()
                    }
                }.dump(4);


            sendResponse(
                clientSocket,
                createResponse(
                    400,
                    "Bad Request",
                    body
                )
            );
        }


        closesocket(
            clientSocket
        );
    }



// ------------------------------------------------------------
// START SERVER (accept loop)
// ------------------------------------------------------------

void HTTPServer::start()
    {
        while(true)
        {
            sockaddr_in clientAddress{};


            int clientAddressSize =
                sizeof(clientAddress);


            SOCKET clientSocket =
                accept(
                    serverSocket,
                    (sockaddr*)&clientAddress,
                    &clientAddressSize
                );


            if(
                clientSocket
                ==
                INVALID_SOCKET
            )
            {
                cout
                    << "Accept failed."
                    << endl;


                continue;
            }


            cout
                << "[Main Thread] Client connected."
                << endl;


            pool.submit(
                [this, clientSocket]()
                {
                    handleClient(
                        clientSocket
                    );
                }
            );
        }
    }

