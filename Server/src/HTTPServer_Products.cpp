#include "HTTPServer.h"

// ============================================================
// /products ROUTE HANDLER (GET / POST / PUT)
// ============================================================

void HTTPServer::handleProducts(
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
            // GET /products
            // TABLE CACHE
            // ------------------------------------------------

            if(id == -1)
            {
                if(
                    productTableCache.get(
                        body
                    )
                )
                {
                    cout
                        << "[TABLE CACHE HIT] "
                        << "products"
                        << endl;
                }
                else
                {
                    cout
                        << "[TABLE CACHE MISS] "
                        << "products"
                        << endl;


                    body =
                        dataStore.getProducts();


                    productTableCache.put(
                        body
                    );
                }
            }

            // ------------------------------------------------
            // GET /products?id=1
            // KEY-VALUE CACHE
            // ------------------------------------------------

            else
            {
                if(
                    productCache.get(
                        id,
                        body
                    )
                )
                {
                    cout
                        << "[KEY CACHE HIT] "
                        << "Product ID = "
                        << id
                        << endl;
                }
                else
                {
                    cout
                        << "[KEY CACHE MISS] "
                        << "Product ID = "
                        << id
                        << endl;


                    body =
                        dataStore.getProductById(
                            id
                        );


                    productCache.put(
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
                !data.contains("price")
                ||
                !data.contains("stock")
            )
            {
                throw runtime_error(
                    "name, price and stock are required"
                );
            }


            string name =
                data["name"].get<string>();


            double price =
                data["price"].get<double>();


            int stock =
                data["stock"].get<int>();


            if(
                price < 0
                ||
                stock < 0
            )
            {
                throw runtime_error(
                    "price and stock cannot be negative"
                );
            }


            string body =
                dataStore.createProduct(
                    name,
                    price,
                    stock
                );


            // ------------------------------------------------
            // NEW PRODUCT ADDED
            //
            // Whole table cache is stale.
            // ------------------------------------------------

            productTableCache.clear();


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
                !data.contains("product_id")
                ||
                !data.contains("name")
                ||
                !data.contains("price")
                ||
                !data.contains("stock")
            )
            {
                throw runtime_error(
                    "product_id, name, price and stock are required"
                );
            }


            int id =
                data["product_id"].get<int>();


            string name =
                data["name"].get<string>();


            double price =
                data["price"].get<double>();


            int stock =
                data["stock"].get<int>();


            string body =
                dataStore.updateProduct(
                    id,
                    name,
                    price,
                    stock
                );


            // ------------------------------------------------
            // IMPORTANT:
            //
            // Updated row:
            //
            // Remove key-value cache
            // Remove table cache
            // ------------------------------------------------

            productCache.remove(
                id
            );


            productTableCache.clear();


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

