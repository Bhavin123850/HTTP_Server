#include "DataStore.h"

// ============================================================
// PRODUCTS
// ============================================================

string DataStore::getProducts()
    {
        auto connection =
            connectionPool.acquire();


        pqxx::work txn(*connection);


        pqxx::result result =
            txn.exec(
                "SELECT product_id, name, price, stock "
                "FROM products "
                "ORDER BY product_id"
            );


        json response =
            json::array();


        for(const auto& row : result)
        {
            json product;


            product["product_id"] =
                row["product_id"].as<int>();


            product["name"] =
                row["name"].as<string>();


            product["price"] =
                row["price"].as<double>();


            product["stock"] =
                row["stock"].as<int>();


            response.push_back(
                product
            );
        }


        txn.commit();


        return response.dump(4);
    }



string DataStore::getProductById(
        int id
    )
    {
        auto connection =
            connectionPool.acquire();


        pqxx::work txn(*connection);


        pqxx::result result =
            txn.exec_params(
                "SELECT product_id, name, price, stock "
                "FROM products "
                "WHERE product_id = $1",
                id
            );


        if(result.empty())
        {
            throw runtime_error(
                "Product not found"
            );
        }


        auto row = result[0];


        json product;


        product["product_id"] =
            row["product_id"].as<int>();


        product["name"] =
            row["name"].as<string>();


        product["price"] =
            row["price"].as<double>();


        product["stock"] =
            row["stock"].as<int>();


        txn.commit();


        return product.dump(4);
    }



string DataStore::createProduct(
        const string& name,
        double price,
        int stock
    )
    {
        auto connection =
            connectionPool.acquire();


        pqxx::work txn(*connection);


        pqxx::result result =
            txn.exec_params(
                "INSERT INTO products "
                "(name, price, stock) "
                "VALUES ($1, $2, $3) "
                "RETURNING product_id, name, price, stock",
                name,
                price,
                stock
            );


        auto row = result[0];


        json product;


        product["product_id"] =
            row["product_id"].as<int>();


        product["name"] =
            row["name"].as<string>();


        product["price"] =
            row["price"].as<double>();


        product["stock"] =
            row["stock"].as<int>();


        txn.commit();


        return product.dump(4);
    }



string DataStore::updateProduct(
        int id,
        const string& name,
        double price,
        int stock
    )
    {
        auto connection =
            connectionPool.acquire();


        pqxx::work txn(*connection);


        pqxx::result result =
            txn.exec_params(
                "UPDATE products "
                "SET name = $1, price = $2, stock = $3 "
                "WHERE product_id = $4 "
                "RETURNING product_id, name, price, stock",
                name,
                price,
                stock,
                id
            );


        if(result.empty())
        {
            throw runtime_error(
                "Product not found"
            );
        }


        auto row = result[0];


        json product;


        product["product_id"] =
            row["product_id"].as<int>();


        product["name"] =
            row["name"].as<string>();


        product["price"] =
            row["price"].as<double>();


        product["stock"] =
            row["stock"].as<int>();


        txn.commit();


        return product.dump(4);
    }

