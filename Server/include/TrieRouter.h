#pragma once

#include "Common.h"



// ============================================================
// 5. TRIE NODE
// ============================================================

class TrieNode
{
public:

    unordered_map<
        char,
        TrieNode*
    > children;


    bool isEndpoint;


    string endpointName;


    TrieNode()
    {
        isEndpoint = false;
    }
};



// ============================================================
// 6. TRIE ROUTER
// ============================================================

class TrieRouter
{
private:

    TrieNode* root;


public:

    TrieRouter()
    {
        root =
            new TrieNode();
    }


    // ========================================================
    // INSERT ROUTE
    // ========================================================

    void insert(
        const string& path,
        const string& endpointName
    )
    {
        TrieNode* current =
            root;


        for(char ch : path)
        {
            if(
                current->children.find(ch)
                ==
                current->children.end()
            )
            {
                current->children[ch] =
                    new TrieNode();
            }


            current =
                current->children[ch];
        }


        current->isEndpoint =
            true;


        current->endpointName =
            endpointName;
    }


    // ========================================================
    // SEARCH ROUTE
    // ========================================================

    string search(
        const string& path
    )
    {
        TrieNode* current =
            root;


        for(char ch : path)
        {
            if(
                current->children.find(ch)
                ==
                current->children.end()
            )
            {
                return "";
            }


            current =
                current->children[ch];
        }


        if(current->isEndpoint)
        {
            return current->endpointName;
        }


        return "";
    }
};
