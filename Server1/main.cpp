#include "HTTPServer.h"

// ============================================================
// SERVER 1 — listens on port 8080
//
// This is one of three identical backend instances behind the
// LoadBalancer. All logic lives in the shared Server/ files;
// this file only picks the port and thread count.
// ============================================================

int main()
{
    HTTPServer server(
        8080,
        8
    );

    server.start();

    return 0;
}
