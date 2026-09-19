#pragma once

#include "Common.h"

// ============================================================
// KEY-VALUE LRU CACHE
//
// Generic least-recently-used cache used for single-record
// lookups, e.g. GET /customers?id=1
// ============================================================

class LRUCache
{
private:

    int capacity;


    // --------------------------------------------------------
    // LIST
    //
    // Front  = Most Recently Used
    // Back   = Least Recently Used
    // --------------------------------------------------------

    list<pair<int, string>> cacheList;


    // --------------------------------------------------------
    // MAP
    //
    // key -> position in list
    // --------------------------------------------------------

    unordered_map<
        int,
        list<pair<int, string>>::iterator
    > cacheMap;


    mutable mutex mtx;


public:

    LRUCache(
        int capacity
    )
        : capacity(capacity)
    {
    }


    // ========================================================
    // GET
    // ========================================================

    bool get(
        int id,
        string& value
    )
    {
        lock_guard<mutex> lock(mtx);


        auto it =
            cacheMap.find(id);


        if(it == cacheMap.end())
        {
            return false;
        }


        // Move accessed item to front
        cacheList.splice(
            cacheList.begin(),
            cacheList,
            it->second
        );


        value =
            it->second->second;


        return true;
    }


    // ========================================================
    // PUT
    // ========================================================

    void put(
        int id,
        const string& value
    )
    {
        lock_guard<mutex> lock(mtx);


        auto it =
            cacheMap.find(id);


        // ----------------------------------------------------
        // Already exists
        // ----------------------------------------------------

        if(it != cacheMap.end())
        {
            it->second->second =
                value;


            cacheList.splice(
                cacheList.begin(),
                cacheList,
                it->second
            );


            return;
        }


        // ----------------------------------------------------
        // New entry
        // ----------------------------------------------------

        cacheList.push_front(
            {id, value}
        );


        cacheMap[id] =
            cacheList.begin();


        // ----------------------------------------------------
        // Remove LRU item if capacity exceeded
        // ----------------------------------------------------

        if(
            static_cast<int>(
                cacheList.size()
            ) > capacity
        )
        {
            auto last =
                prev(
                    cacheList.end()
                );


            cacheMap.erase(
                last->first
            );


            cacheList.pop_back();
        }
    }


    // ========================================================
    // REMOVE ONE KEY
    // ========================================================

    void remove(
        int id
    )
    {
        lock_guard<mutex> lock(mtx);


        auto it =
            cacheMap.find(id);


        if(it == cacheMap.end())
        {
            return;
        }


        cacheList.erase(
            it->second
        );


        cacheMap.erase(
            it
        );


        cout
            << "[KEY CACHE] Removed ID = "
            << id
            << endl;
    }


    // ========================================================
    // CLEAR ENTIRE KEY-VALUE CACHE
    // ========================================================

    void clear()
    {
        lock_guard<mutex> lock(mtx);


        cacheList.clear();

        cacheMap.clear();


        cout
            << "[KEY CACHE] Entire cache cleared."
            << endl;
    }
};
