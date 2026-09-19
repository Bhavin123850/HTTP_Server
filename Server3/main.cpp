#include "HTTPServer.h"

// ============================================================
// SERVER 3 — listens on port 9000
//
// This is one of three identical backend instances behind the
// LoadBalancer. All logic lives in the shared Server/ files;
// this file only picks the port and thread count.
// ============================================================

int main()
{
    HTTPServer server(
        9000,
        8
    );

    server.start();

    return 0;
}
