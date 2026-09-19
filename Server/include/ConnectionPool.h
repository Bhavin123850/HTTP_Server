#pragma once

#include "Common.h"

// ============================================================
// DATABASE CONNECTION POOL
//
// Manages a fixed pool of pqxx PostgreSQL connections so that
// concurrent requests don't each open their own DB connection.
// ============================================================

class ConnectionPool
{
private:

    vector<unique_ptr<pqxx::connection>> connections;

    queue<pqxx::connection*> availableConnections;

    mutex mtx;

    condition_variable cv;


public:

    // ========================================================
    // CONNECTION LEASE
    // ========================================================

    class ConnectionLease
    {
    private:

        ConnectionPool* pool;

        pqxx::connection* connection;


    public:

        ConnectionLease(
            ConnectionPool* pool,
            pqxx::connection* connection
        )
            : pool(pool),
              connection(connection)
        {
        }


        ConnectionLease(const ConnectionLease&) = delete;


        ConnectionLease& operator=(
            const ConnectionLease&
        ) = delete;


        ConnectionLease(
            ConnectionLease&& other
        ) noexcept
        {
            pool = other.pool;

            connection = other.connection;

            other.pool = nullptr;

            other.connection = nullptr;
        }


        ConnectionLease& operator=(
            ConnectionLease&& other
        ) noexcept
        {
            if(this != &other)
            {
                release();

                pool = other.pool;

                connection = other.connection;

                other.pool = nullptr;

                other.connection = nullptr;
            }

            return *this;
        }


        ~ConnectionLease()
        {
            release();
        }


        pqxx::connection& get()
        {
            return *connection;
        }


        pqxx::connection* operator->()
        {
            return connection;
        }


        pqxx::connection& operator*()
        {
            return *connection;
        }


    private:

        void release()
        {
            if(
                pool != nullptr &&
                connection != nullptr
            )
            {
                pool->release(connection);

                pool = nullptr;

                connection = nullptr;
            }
        }
    };


    // ========================================================
    // CONSTRUCTOR
    // ========================================================

    ConnectionPool(
        const string& connectionString,
        int poolSize
    )
    {
        cout
            << "[DB POOL] Creating "
            << poolSize
            << " connections..."
            << endl;


        for(int i = 0; i < poolSize; i++)
        {
            auto connection =
                make_unique<pqxx::connection>(
                    connectionString
                );


            if(!connection->is_open())
            {
                throw runtime_error(
                    "Failed to open PostgreSQL connection"
                );
            }


            pqxx::connection* rawPointer =
                connection.get();


            connections.push_back(
                move(connection)
            );


            availableConnections.push(
                rawPointer
            );
        }


        cout
            << "[DB POOL] "
            << poolSize
            << " connections ready."
            << endl;
    }


    // ========================================================
    // ACQUIRE CONNECTION
    // ========================================================

    ConnectionLease acquire()
    {
        unique_lock<mutex> lock(mtx);


        cv.wait(
            lock,
            [this]()
            {
                return !availableConnections.empty();
            }
        );


        pqxx::connection* connection =
            availableConnections.front();


        availableConnections.pop();


        cout
            << "[DB POOL] Connection acquired. "
            << "Available = "
            << availableConnections.size()
            << endl;


        return ConnectionLease(
            this,
            connection
        );
    }


private:

    // ========================================================
    // RELEASE CONNECTION
    // ========================================================

    void release(
        pqxx::connection* connection
    )
    {
        {
            lock_guard<mutex> lock(mtx);


            availableConnections.push(
                connection
            );


            cout
                << "[DB POOL] Connection released. "
                << "Available = "
                << availableConnections.size()
                << endl;
        }


        cv.notify_one();
    }
};
