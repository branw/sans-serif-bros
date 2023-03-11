#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include "env.h"
#include "db.h"
#include "server.h"

#define USAGE "usage: ssb [-hvs] [-d path/to/db] [-p port]\n"
#define VERSION "0.1"

struct termios orig_termios;

volatile sig_atomic_t running = true;

void handle_signal(int signal_num) {
    fprintf(stderr, "Got signal %d\n", signal_num);

    running = false;
}

void enter_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void exit_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void show_cursor(void) {
    printf("\x1b[?25h");
}

void disable_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void restore_terminal(void) {
    exit_raw_mode();
    show_cursor();
}

int run_standalone(struct db *db) {
    // Disable terminal processing and receive raw ANSI sequences like with Telnet
    enter_raw_mode();
    atexit(restore_terminal);

    // Don't block on reads to stdin
    disable_blocking(STDIN_FILENO);

    struct env env = {.db=db};

    struct state state;
    state_create(&state);

    while (running) {
        char buf[512];
        ssize_t read_len;
        while ((read_len = read(STDIN_FILENO, buf, 512)) > 0) {
            terminal_parse(&state.terminal, buf, read_len);
        }

        if (state_update(&state, &env)) {
            // Flush and send output data
            size_t write_len;
            while (terminal_flush(&state.terminal, buf, 512, &write_len)) {
                write(STDOUT_FILENO, buf, write_len);
            }
        }
        else {
            running = false;
        }
    }

    printf("Shutting down standalone mode...\n");

    return EXIT_SUCCESS;
}

int run_server(struct db *db, char *service) {
    // Launch the server
    struct server server;
    if (!server_create(&server, service)) {
        fprintf(stderr, "failed to create server\n");

        return EXIT_FAILURE;
    }

    struct env env = {.db=db, .server=&server};

    // Accept new connections and update existing ones
    while (running && server_update(&server)) {
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
            if (alive && state_update(session->state, &env)) {
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

    printf("Shutting down server...\n");

    server_destroy(&server);

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    char *port = "telnet", *db_path = "levels";
    bool standalone = false;

    // Parse command-line arguments
    int opt;
    while ((opt = getopt(argc, argv, "hvd:p:s")) != -1) {
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
    if (!db_create(&db, db_path)) {
        fprintf(stderr, "failed to create db\n");
        return EXIT_FAILURE;
    }

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = handle_signal;
    sigaction(SIGTERM, &action, NULL);

    int rc = standalone ? run_standalone(&db) : run_server(&db, port);

    printf("Saving database\n");

    db_destroy(&db);

    printf("Shutdown complete\n");

    return rc;
}
