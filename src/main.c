#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "db.h"
#include "server.h"

#define USAGE "usage: ssb [-hv] [-d path/to/db] [-p port]\n"
#define VERSION GIT_COMMIT_HASH

int main(int argc, char *argv[]) {
    char *port = NULL, *db_path = NULL;
    bool standalone = false;

    // Parse command-line arguments
    int opt;
    while ((opt = getopt(argc, argv, "hvp:")) != -1) {
        switch (opt) {
            case 'd': {
                db_path = optarg;
                break;
            }

            case 'p': {
                port = optarg;
                break;
            }

            case 's':
                standalone = true;
                break;

            case 'h': {
                printf("ssb (sans serif bros) " VERSION " - a Telnet platformer\n"
                USAGE
                "    -d path         Directory of level data (default: \"levels\")\n"
                "    -p port         Server port number or name (default: \"telnet\" (23))\n"
                "    -s              Disable the server and play locally only\n"
                "    -h              Show this help message\n"
                "    -v              Show the version\n");
                return EXIT_SUCCESS;
            }

            case 'v': {
                printf("ssb " VERSION "\n");
                return EXIT_SUCCESS;
            }

            case '?':
            default: {
                fprintf(stderr, USAGE);
                return EXIT_FAILURE;
            }
        }
    }

    // Load the level metadata
    struct db db;
    if (!db_create(&db, db_path ? db_path : "levels")) {
        fprintf(stderr, "failed to create db\n");
        return EXIT_FAILURE;
    }


    if (standalone) {
        /*
        struct state state;
        while (state_update(&db, &state)) {

        }
         */

        fprintf(stderr, "Unimplemented\n");
        return EXIT_FAILURE;
    }

    // Launch the server
    struct server server;
    if (!server_create(&server, port ? port : "telnet")) {
        fprintf(stderr, "failed to create server\n");
        return EXIT_FAILURE;
    }

    // Accept new connections and update existing ones
    while (server_update(&server)) {
        // Update each session
        struct session *session = NULL;
        while (server_next_session(&server, &session)) {
            // Poll the connection for data and parse it
            bool alive;
            char buf[512];
            size_t len;
            while ((alive = session_receive(session, buf, 512, &len)) && len > 0) {
                terminal_parse(&session->state->terminal, buf, len);
            }

            // Try to update the state
            if (alive && state_update(session->state, &db)) {
                // Flush and send output data
                while (terminal_flush(&session->state->terminal, buf, 512, &len)) {
                    session_send(session, buf, len);
                }
            }
            // The connection was already disconnected or should be disconnected
            else {
                // Store the previous session so we don't break the iterator
                struct session *prev = session->prev;
                // Kill the connection
                server_disconnect_session(&server, session);
                session = prev;
            }
        }
    }

    server_destroy(&server);

    return EXIT_SUCCESS;
}
