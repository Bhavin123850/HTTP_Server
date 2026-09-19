#include "HTTPServer.h"

// ============================================================
// SERVER 2 — listens on port 8000
//
// This is one of three identical backend instances behind the
// LoadBalancer. All logic lives in the shared Server/ files;
// this file only picks the port and thread count.
// ============================================================

int main()
{
    HTTPServer server(
        8000,
        8
    );

    server.start();

    return 0;
}
