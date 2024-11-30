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
#include "log.h"

#define USAGE "usage: ssb [-hvs] [-d path/to/db] [-p port]\n"
#define VERSION "0.1"

#define DEFAULT_PORT "23"
#define DEFAULT_DB_PATH "ssb.sqlite"
#define DEFAULT_LEVEL_PATH "levels"

struct termios orig_termios;

volatile sig_atomic_t running = true;

void handle_signal(int signal_num) {
    if (signal_num == SIGKILL) {
        LOG_INFO("Got SIGKILL");
        running = false;
    }
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

        if (!state_update(&state, &env)) {
            running = false;
        }

        // Flush and send output data
        size_t write_len;
        while (terminal_flush(&state.terminal, buf, 512, &write_len)) {
            size_t const written_len = write(STDOUT_FILENO, buf, write_len);
            if (written_len != write_len) {
                LOG_ERROR("Tried writing %d, but only wrote %d", write_len, written_len);
            }
        }
    }

    LOG_INFO("Shutting down standalone mode...");

    return EXIT_SUCCESS;
}

int run_server(struct db *db, char *service) {
    // Launch the server
    struct server server;
    if (!server_create(&server, service)) {
        LOG_ERROR("Failed to create server");

        return EXIT_FAILURE;
    }

    struct env env = {.db=db, .server=&server};

    // Accept new connections and update existing ones
    while (running && server_update(&server)) {
        // Update each session
        struct session *session = NULL;
        while (server_next_session(&server, &session)) {
            log_push_context(session->id);

            // Poll the connection for data and parse it
            bool alive;
            char buf[512];
            size_t len;
            while ((alive = session_receive(session, buf, 512, &len)) && len > 0) {
                terminal_parse(&session->state->terminal, buf, len);
            }

            // Try to update the state
            bool const keep_alive = state_update(session->state, &env);

            if (alive) {
                // Flush and send output data if we're alive, even when we're
                // about to close the connection
                while (terminal_flush(&session->state->terminal, buf, 512, &len)) {
                    session_send(session, buf, len);
                }
            }
            // The connection was already disconnected or should be disconnected
            if (!alive || !keep_alive) {
                // Store the previous session, so we don't break the iterator
                struct session *prev = session->prev;
                // Kill the connection
                server_disconnect_session(&server, session);
                session = prev;
            }

            log_pop_context();
        }
    }

    LOG_INFO("Shutting down server...");

    server_destroy(&server);

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    char *port = DEFAULT_PORT;
    char *db_path = DEFAULT_DB_PATH;
    char *levels_path = DEFAULT_LEVEL_PATH;
    bool standalone = false;

    // Parse command-line arguments
    int opt;
    while ((opt = getopt(argc, argv, "hvd:l:p:s")) != -1) {
        switch (opt) {
            case 'd': {
                db_path = optarg;
                break;
            }

            case 'l': {
                levels_path = optarg;
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
                "    -d path         Path to database (default: \"" DEFAULT_DB_PATH "\")\n"
                "    -l path         Path of levels to load for new databases (default: \"" DEFAULT_LEVEL_PATH "\")\n"
                "    -p port         Server port number or name (default: \"" DEFAULT_PORT "\")\n"
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

    LOG_DEBUG("Initializing ssb version " VERSION);

    if (standalone && !strncmp(port, DEFAULT_PORT, sizeof(DEFAULT_PORT))) {
        LOG_ERROR("Port cannot be specified in standalone mode");
        return EXIT_FAILURE;
    }

    // Load the level metadata
    struct db db;
    if (!db_create(&db, db_path, levels_path)) {
        LOG_ERROR("Failed to create DB");
        return EXIT_FAILURE;
    }

    // Install a signal handler to gracefully shutdown
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = handle_signal;
    sigaction(SIGTERM, &action, NULL);

    LOG_INFO("Running in %s mode", standalone ? "standalone" : "server");
    int rc = standalone ? run_standalone(&db) : run_server(&db, port);

    LOG_INFO("Closing database");
    db_destroy(&db);

    LOG_INFO("Shutdown complete. Exiting with code %d", rc);
    return rc;
}
