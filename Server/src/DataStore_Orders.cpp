#include "DataStore.h"

// ============================================================
// ORDERS
// ============================================================

string DataStore::getOrders()
    {
        auto connection =
            connectionPool.acquire();


        pqxx::work txn(*connection);


        pqxx::result result =
            txn.exec(
                "SELECT order_id, customer_id, "
                "product_id, quantity "
                "FROM orders "
                "ORDER BY order_id"
            );


        json response =
            json::array();


        for(const auto& row : result)
        {
            json order;


            order["order_id"] =
                row["order_id"].as<int>();


            order["customer_id"] =
                row["customer_id"].as<int>();


            order["product_id"] =
                row["product_id"].as<int>();


            order["quantity"] =
                row["quantity"].as<int>();


            response.push_back(
                order
            );
        }


        txn.commit();


        return response.dump(4);
    }



string DataStore::getOrderById(
        int id
    )
    {
        auto connection =
            connectionPool.acquire();


        pqxx::work txn(*connection);


        pqxx::result result =
            txn.exec_params(
                "SELECT order_id, customer_id, "
                "product_id, quantity "
                "FROM orders "
                "WHERE order_id = $1",
                id
            );


        if(result.empty())
        {
            throw runtime_error(
                "Order not found"
            );
        }


        auto row = result[0];


        json order;


        order["order_id"] =
            row["order_id"].as<int>();


        order["customer_id"] =
            row["customer_id"].as<int>();


        order["product_id"] =
            row["product_id"].as<int>();


        order["quantity"] =
            row["quantity"].as<int>();


        txn.commit();


        return order.dump(4);
    }



string DataStore::createOrder(
        int customerId,
        int productId,
        int quantity
    )
    {
        auto connection =
            connectionPool.acquire();


        pqxx::work txn(*connection);


        pqxx::result result =
            txn.exec_params(
                "INSERT INTO orders "
                "(customer_id, product_id, quantity) "
                "VALUES ($1, $2, $3) "
                "RETURNING order_id, customer_id, "
                "product_id, quantity",
                customerId,
                productId,
                quantity
            );


        auto row = result[0];


        json order;


        order["order_id"] =
            row["order_id"].as<int>();


        order["customer_id"] =
            row["customer_id"].as<int>();


        order["product_id"] =
            row["product_id"].as<int>();


        order["quantity"] =
            row["quantity"].as<int>();


        txn.commit();


        return order.dump(4);
    }



string DataStore::updateOrder(
        int id,
        int customerId,
        int productId,
        int quantity
    )
    {
        auto connection =
            connectionPool.acquire();


        pqxx::work txn(*connection);


        pqxx::result result =
            txn.exec_params(
                "UPDATE orders "
                "SET customer_id = $1, "
                "product_id = $2, "
                "quantity = $3 "
                "WHERE order_id = $4 "
                "RETURNING order_id, customer_id, "
                "product_id, quantity",
                customerId,
                productId,
                quantity,
                id
            );


        if(result.empty())
        {
            throw runtime_error(
                "Order not found"
            );
        }


        auto row = result[0];


        json order;


        order["order_id"] =
            row["order_id"].as<int>();


        order["customer_id"] =
            row["customer_id"].as<int>();


        order["product_id"] =
            row["product_id"].as<int>();


        order["quantity"] =
            row["quantity"].as<int>();


        txn.commit();


        return order.dump(4);
    }

