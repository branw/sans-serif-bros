#define _WIN32_WINNT _WIN32_WINNT_WIN8

#include <Winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdbool.h>

#include "server.h"
#include "config.h"

bool server_create(struct server *server) {
    // Initialize WSA
    static bool wsa_initialized = false;
    if (!wsa_initialized) {
        WSADATA wsa_data;

        int res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (res) {
            printf("WSAStartup failed (%d)\n", res);
            return false;
        }

        wsa_initialized = true;
    }

    struct addrinfo *addr = NULL;
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve a Telnet address
    int res = getaddrinfo(NULL, "telnet", &hints, &addr);
    if (res) {
        fprintf(stderr, "getaddrinfo failed (%d)\n", res);
        return false;
    }

    // Create a new socket
    SOCKET sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "socket failed (%d)\n", WSAGetLastError());
        freeaddrinfo(addr);
        return false;
    }

    // Bind the socket to the address
    res = bind(sock, addr->ai_addr, (int) addr->ai_addrlen);
    freeaddrinfo(addr);
    if (res == SOCKET_ERROR) {
        fprintf(stderr, "bind failed (%d)\n", WSAGetLastError());
        closesocket(sock);
        return false;
    }

    // Start listening on the socket
    res = listen(sock, SOMAXCONN);
    if (res == SOCKET_ERROR) {
        fprintf(stderr, "listen failed (%d)\n", WSAGetLastError());
        closesocket(sock);
        return false;
    }

    server->listen_sock = sock;
    server->sessions = NULL;
    server->num_sessions = server->total_sessions = 0;

    return true;
}


void server_accept_session(struct server *server) {
    // Accept the new connection
    SOCKET sock = accept(server->listen_sock, NULL, NULL);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "accept failed (%d)\n", WSAGetLastError());
        closesocket(sock);
        return;
    }

    struct session *sess = malloc(sizeof(*sess));
    sess->id = server->total_sessions++;
    session_create(sess, sock);

    // If there are too many concurrent sessions, disconnect this one
    if (server->num_sessions >= MAX_SESSIONS) {
        fprintf(stderr, "max sessions reached (%d)\n", MAX_SESSIONS);
        terminal_write(sess, "Too many users are on at the moment; try again later!\n\r");
        session_shutdown(sess);
        free(sess);
        return;
    }

    // Add the session to the list
    sess->prev = sess->next = NULL;
    if (server->sessions == NULL) {
        server->sessions = sess;
    } else {
        sess->next = server->sessions;
        server->sessions->prev = sess;
        server->sessions = sess;
    }
    server->num_sessions++;
}

bool server_next_session(struct server *server, struct session **sess) {
    if (*sess == NULL) {
        if (server->sessions == NULL) {
            return false;
        }
        *sess = server->sessions;
    } else {
        if ((*sess)->next == NULL) {
            return false;
        }
        *sess = (*sess)->next;
    }
    return true;
}

void server_disconnect_session(struct server *server, struct session *sess) {
    session_shutdown(sess);

    // Remove the session from the list
    if (sess->prev != NULL) {
        sess->prev->next = sess->next;
    }
    if (sess->next != NULL) {
        sess->next->prev = sess->prev;
    }
    if (server->sessions == sess) {
        server->sessions = NULL;
    }
    server->num_sessions--;
    free(sess);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

void server_run(struct server *server) {
    for (;;) {
        // Read from the listening socket
        struct fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(server->listen_sock, &read_set);

        // Don't block (for very long)
        struct timeval timeout = {.tv_sec=0, .tv_usec=1};

        // Check if a new connection is available
        if (select(FD_SETSIZE, &read_set, NULL, NULL, &timeout)) {
            server_accept_session(server);
        }

        // Update each session
        struct session *sess = NULL;
        while (server_next_session(server, &sess)) {
            if (!session_update(sess)) {
                // Move the iterator back to the previous, because the
                // current session is going to be deleted
                struct session *temp = sess->prev;
                server_disconnect_session(server, sess);
                sess = temp;
            }
        }
    }
}

#pragma clang diagnostic pop
