#pragma once

#include "Common.h"

// ============================================================
// DATABASE CONFIGURATION
//
// All three server instances (server1 / server2 / server3)
// connect to the same PostgreSQL database. Only the listening
// port differs between them (see each server's main.cpp).
// ============================================================

const string DB_CONNECTION =
    "host=127.0.0.1 "
    "port=5432 "
    "dbname=HttpServerDB "
    "user=postgres "
    "password=Bhavin@.712";
