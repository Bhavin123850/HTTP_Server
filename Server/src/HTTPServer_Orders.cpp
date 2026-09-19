#include "HTTPServer.h"

// ============================================================
// /orders ROUTE HANDLER (GET / POST / PUT)
// ============================================================

void HTTPServer::handleOrders(
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
            // GET /orders
            // TABLE CACHE
            // ------------------------------------------------

            if(id == -1)
            {
                if(
                    orderTableCache.get(
                        body
                    )
                )
                {
                    cout
                        << "[TABLE CACHE HIT] "
                        << "orders"
                        << endl;
                }
                else
                {
                    cout
                        << "[TABLE CACHE MISS] "
                        << "orders"
                        << endl;


                    body =
                        dataStore.getOrders();


                    orderTableCache.put(
                        body
                    );
                }
            }

            // ------------------------------------------------
            // GET /orders?id=1
            // KEY-VALUE CACHE
            // ------------------------------------------------

            else
            {
                if(
                    orderCache.get(
                        id,
                        body
                    )
                )
                {
                    cout
                        << "[KEY CACHE HIT] "
                        << "Order ID = "
                        << id
                        << endl;
                }
                else
                {
                    cout
                        << "[KEY CACHE MISS] "
                        << "Order ID = "
                        << id
                        << endl;


                    body =
                        dataStore.getOrderById(
                            id
                        );


                    orderCache.put(
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
                !data.contains("customer_id")
                ||
                !data.contains("product_id")
                ||
                !data.contains("quantity")
            )
            {
                throw runtime_error(
                    "customer_id, product_id and quantity are required"
                );
            }


            int customerId =
                data["customer_id"].get<int>();


            int productId =
                data["product_id"].get<int>();


            int quantity =
                data["quantity"].get<int>();


            if(quantity <= 0)
            {
                throw runtime_error(
                    "quantity must be greater than 0"
                );
            }


            string body =
                dataStore.createOrder(
                    customerId,
                    productId,
                    quantity
                );


            // ------------------------------------------------
            // NEW ORDER ADDED
            //
            // Whole orders table cache is stale.
            // ------------------------------------------------

            orderTableCache.clear();


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
                !data.contains("order_id")
                ||
                !data.contains("customer_id")
                ||
                !data.contains("product_id")
                ||
                !data.contains("quantity")
            )
            {
                throw runtime_error(
                    "order_id, customer_id, product_id and quantity are required"
                );
            }


            int id =
                data["order_id"].get<int>();


            int customerId =
                data["customer_id"].get<int>();


            int productId =
                data["product_id"].get<int>();


            int quantity =
                data["quantity"].get<int>();


            if(quantity <= 0)
            {
                throw runtime_error(
                    "quantity must be greater than 0"
                );
            }


            string body =
                dataStore.updateOrder(
                    id,
                    customerId,
                    productId,
                    quantity
                );


            // ------------------------------------------------
            // IMPORTANT:
            //
            // Remove:
            //
            // 1. Updated order from key-value cache
            // 2. Complete orders table from table cache
            // ------------------------------------------------

            orderCache.remove(
                id
            );


            orderTableCache.clear();


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



        sendResponse(
            clientSocket,
            createResponse(
                405,
                "Method Not Allowed",
                R"({"error":"Method not supported"})"
            )
        );
    }

