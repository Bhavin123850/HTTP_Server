#include "HTTPServer.h"

// ============================================================
// /customers ROUTE HANDLER (GET / POST / PUT)
// ============================================================

void HTTPServer::handleCustomers(
        const HTTPRequest& request,
        SOCKET clientSocket
    )
    {
        // ====================================================
        // GET
        // ====================================================

        if(
            request.method
            ==
            "GET"
        )
        {
            int id =
                getIdFromQuery(
                    request.path
                );


            string body;


            // ------------------------------------------------
            // GET /customers
            //
            // TABLE CACHE
            // ------------------------------------------------

            if(id == -1)
            {
                // --------------------------------------------
                // TABLE CACHE HIT
                // --------------------------------------------

                if(
                    customerTableCache.get(
                        body
                    )
                )
                {
                    cout
                        << "[TABLE CACHE HIT] "
                        << "customers"
                        << endl;
                }

                // --------------------------------------------
                // TABLE CACHE MISS
                // --------------------------------------------

                else
                {
                    cout
                        << "[TABLE CACHE MISS] "
                        << "customers"
                        << endl;


                    body =
                        dataStore.getCustomers();


                    // Store complete table
                    // in table cache

                    customerTableCache.put(
                        body
                    );
                }
            }

            // ------------------------------------------------
            // GET /customers?id=1
            //
            // KEY-VALUE CACHE
            // ------------------------------------------------

            else
            {
                if(
                    customerCache.get(
                        id,
                        body
                    )
                )
                {
                    cout
                        << "[KEY CACHE HIT] "
                        << "Customer ID = "
                        << id
                        << endl;
                }
                else
                {
                    cout
                        << "[KEY CACHE MISS] "
                        << "Customer ID = "
                        << id
                        << endl;


                    body =
                        dataStore.getCustomerById(
                            id
                        );


                    customerCache.put(
                        id,
                        body
                    );
                }
            }


            sendResponse(
                clientSocket,
                createResponse(
                    200,
                    "OK",
                    body
                )
            );


            return;
        }



        // ====================================================
        // POST
        // ====================================================

        if(
            request.method
            ==
            "POST"
        )
        {
            json data =
                json::parse(
                    request.body
                );


            if(
                !data.contains("name")
                ||
                !data.contains("email")
            )
            {
                throw runtime_error(
                    "name and email are required"
                );
            }


            string name =
                data["name"].get<string>();


            string email =
                data["email"].get<string>();


            string body =
                dataStore.createCustomer(
                    name,
                    email
                );


            // ------------------------------------------------
            // IMPORTANT:
            //
            // New row added.
            // Complete table cache is now stale.
            // ------------------------------------------------

            customerTableCache.clear();


            sendResponse(
                clientSocket,
                createResponse(
                    201,
                    "Created",
                    body
                )
            );


            return;
        }



        // ====================================================
        // PUT
        // ====================================================

        if(
            request.method
            ==
            "PUT"
        )
        {
            json data =
                json::parse(
                    request.body
                );


            if(
                !data.contains("customer_id")
                ||
                !data.contains("name")
                ||
                !data.contains("email")
            )
            {
                throw runtime_error(
                    "customer_id, name and email are required"
                );
            }


            int id =
                data["customer_id"].get<int>();


            string name =
                data["name"].get<string>();


            string email =
                data["email"].get<string>();


            string body =
                dataStore.updateCustomer(
                    id,
                    name,
                    email
                );


            // ------------------------------------------------
            // IMPORTANT:
            //
            // 1. Remove old key-value entry
            // 2. Remove complete table cache
            //
            // Both caches are now stale.
            // ------------------------------------------------

            customerCache.remove(
                id
            );


            customerTableCache.clear();


            sendResponse(
                clientSocket,
                createResponse(
                    200,
                    "OK",
                    body
                )
            );


            return;
        }



        // ====================================================
        // METHOD NOT ALLOWED
        // ====================================================

        sendResponse(
            clientSocket,
            createResponse(
                405,
                "Method Not Allowed",
                R"({"error":"Method not supported"})"
            )
        );
    }

