#include "DataStore.h"

// ============================================================
// CUSTOMERS
// ============================================================

string DataStore::getCustomers()
    {
        auto connection =
            connectionPool.acquire();


        pqxx::work txn(*connection);


        pqxx::result result =
            txn.exec(
                "SELECT customer_id, name, email "
                "FROM customers "
                "ORDER BY customer_id"
            );


        json response =
            json::array();


        for(const auto& row : result)
        {
            json customer;


            customer["customer_id"] =
                row["customer_id"].as<int>();


            customer["name"] =
                row["name"].as<string>();


            customer["email"] =
                row["email"].as<string>();


            response.push_back(
                customer
            );
        }


        txn.commit();


        return response.dump(4);
    }



string DataStore::getCustomerById(
        int id
    )
    {
        auto connection =
            connectionPool.acquire();


        pqxx::work txn(*connection);


        pqxx::result result =
            txn.exec_params(
                "SELECT customer_id, name, email "
                "FROM customers "
                "WHERE customer_id = $1",
                id
            );


        if(result.empty())
        {
            throw runtime_error(
                "Customer not found"
            );
        }


        auto row = result[0];


        json customer;


        customer["customer_id"] =
            row["customer_id"].as<int>();


        customer["name"] =
            row["name"].as<string>();


        customer["email"] =
            row["email"].as<string>();


        txn.commit();


        return customer.dump(4);
    }



string DataStore::createCustomer(
        const string& name,
        const string& email
    )
    {
        auto connection =
            connectionPool.acquire();


        pqxx::work txn(*connection);


        pqxx::result result =
            txn.exec_params(
                "INSERT INTO customers "
                "(name, email) "
                "VALUES ($1, $2) "
                "RETURNING customer_id, name, email",
                name,
                email
            );


        auto row = result[0];


        json customer;


        customer["customer_id"] =
            row["customer_id"].as<int>();


        customer["name"] =
            row["name"].as<string>();


        customer["email"] =
            row["email"].as<string>();


        txn.commit();


        return customer.dump(4);
    }



string DataStore::updateCustomer(
        int id,
        const string& name,
        const string& email
    )
    {
        auto connection =
            connectionPool.acquire();


        pqxx::work txn(*connection);


        pqxx::result result =
            txn.exec_params(
                "UPDATE customers "
                "SET name = $1, email = $2 "
                "WHERE customer_id = $3 "
                "RETURNING customer_id, name, email",
                name,
                email,
                id
            );


        if(result.empty())
        {
            throw runtime_error(
                "Customer not found"
            );
        }


        auto row = result[0];


        json customer;


        customer["customer_id"] =
            row["customer_id"].as<int>();


        customer["name"] =
            row["name"].as<string>();


        customer["email"] =
            row["email"].as<string>();


        txn.commit();


        return customer.dump(4);
    }

