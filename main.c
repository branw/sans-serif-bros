#include "server.h"

int main() {
    struct server server;
    if (!server_create(&server)) {
        return 1;
    }

    server_run(&server);

    return 0;
}
