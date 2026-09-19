#pragma once

#include "Common.h"



// ============================================================
// 4. TABLE CACHE
// ============================================================
//
// This cache stores the complete result of:
//
// GET /customers
// GET /products
// GET /orders
//
// Example:
//
// customers table:
//
// [
//   {...},
//   {...},
//   {...}
// ]
//
// ============================================================

class TableCache
{
private:

    string cachedData;

    bool valid;


    mutable mutex mtx;


public:

    TableCache()
        : valid(false)
    {
    }


    // ========================================================
    // GET TABLE FROM CACHE
    // ========================================================

    bool get(
        string& value
    )
    {
        lock_guard<mutex> lock(mtx);


        if(!valid)
        {
            return false;
        }


        value =
            cachedData;


        return true;
    }


    // ========================================================
    // PUT TABLE INTO CACHE
    // ========================================================

    void put(
        const string& value
    )
    {
        lock_guard<mutex> lock(mtx);


        cachedData =
            value;


        valid = true;
    }


    // ========================================================
    // INVALIDATE TABLE CACHE
    // ========================================================
    //
    // IMPORTANT:
    //
    // Whenever ANY row of the table changes,
    // we remove the WHOLE table cache.
    //
    // ========================================================

    void clear()
    {
        lock_guard<mutex> lock(mtx);


        cachedData.clear();

        valid = false;


        cout
            << "[TABLE CACHE] Table cache invalidated."
            << endl;
    }


    // ========================================================
    // CHECK WHETHER CACHE IS VALID
    // ========================================================

    bool isValid() const
    {
        lock_guard<mutex> lock(mtx);

        return valid;
    }
};
