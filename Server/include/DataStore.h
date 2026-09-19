#pragma once

#include "Common.h"
#include "ConnectionPool.h"

// ============================================================
// DATA STORE
//
// Holds all SQL access (customers, products, orders) behind a
// simple interface. Every method acquires a pooled connection,
// runs a transaction, and returns a JSON string.
//
// Method bodies live in:
//   - DataStore_Customers.cpp
//   - DataStore_Products.cpp
//   - DataStore_Orders.cpp
// ============================================================

class DataStore
{
private:

    ConnectionPool& connectionPool;


public:

    DataStore(
        ConnectionPool& pool
    )
        : connectionPool(pool)
    {
    }


    // ========================================================
    // CUSTOMERS
    // ========================================================

    string getCustomers();

    string getCustomerById(
        int id
    );

    string createCustomer(
        const string& name,
        const string& email
    );

    string updateCustomer(
        int id,
        const string& name,
        const string& email
    );


    // ========================================================
    // PRODUCTS
    // ========================================================

    string getProducts();

    string getProductById(
        int id
    );

    string createProduct(
        const string& name,
        double price,
        int stock
    );

    string updateProduct(
        int id,
        const string& name,
        double price,
        int stock
    );


    // ========================================================
    // ORDERS
    // ========================================================

    string getOrders();

    string getOrderById(
        int id
    );

    string createOrder(
        int customerId,
        int productId,
        int quantity
    );

    string updateOrder(
        int id,
        int customerId,
        int productId,
        int quantity
    );
};
