#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "server.h"

bool server_create(struct server *server, char *service) {
    struct addrinfo *addr = NULL;
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve a Telnet address
    int result = getaddrinfo(NULL, service, &hints, &addr);
    if (result) {
        fprintf(stderr, "getaddrinfo failed (%d)\n", result);
        return false;
    }

    // Create a new socket
    int sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (sock == -1) {
        fprintf(stderr, "socket failed (%d: %s)\n", errno, strerror(errno));
        freeaddrinfo(addr);
        return false;
    }

    // Bind the socket to the address
    result = bind(sock, addr->ai_addr, (int) addr->ai_addrlen);
    freeaddrinfo(addr);
    if (result == -1) {
        fprintf(stderr, "bind failed (%d: %s)\n", errno, strerror(errno));
        goto failure;
    }

    result = listen(sock, SOMAXCONN);
    if (result == -1) {
        fprintf(stderr, "listen failed (%d: %s)\n", errno, strerror(errno));
        goto failure;
    }

    server->socket = sock;

    server->sessions = NULL;
    server->num_sessions = 0;

    return true;

    failure:
    close(sock);
    return false;
}

void server_destroy(struct server *server) {
    struct session *session = NULL;
    while (server_next_session(server, &session)) {
        struct session *prev = session->prev;
        server_disconnect_session(server, session);
        session = prev;
    }

    // Prevent anymore sending on the socket
    int result = shutdown(server->socket, SHUT_WR);
    if (result == -1) {
        fprintf(stderr, "shutdown failed (%d: %s)\n", errno, strerror(errno));
    }

    // Close the socket
    close(server->socket);
}

bool server_update(struct server *server) {
    // Check if data can be read from the socket we're listening to
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(server->socket, &read_set);

    // Block for a trivial amount of time
    struct timeval timeout = {.tv_sec=0, .tv_usec=1};

    int result = select(FD_SETSIZE, &read_set, NULL, NULL, &timeout);
    if (result == -1) {
        fprintf(stderr, "select failed (%d: %s)\n", errno, strerror(errno));
        return false;
    }
    // There aren't any new connections
    else if (result == 0) {
        return true;
    }

    // Accept the new connection
    int sock = accept(server->socket, NULL, NULL);
    if (sock == -1) {
        fprintf(stderr, "accept failed (%d: %s)\n", errno, strerror(errno));
        return false;
    }

    // Create a new session
    struct session *session = malloc(sizeof(*session));
    if (!session_create(session, sock)) {
        fprintf(stderr, "session_create failed\n");
        return false;
    }

    // Add the session to the list
    session->prev = session->next = NULL;
    if (server->sessions == NULL) {
        server->sessions = session;
    } else {
        session->next = server->sessions;
        server->sessions->prev = session;
        server->sessions = session;
    }
    server->num_sessions++;

    return true;
}

void server_disconnect_session(struct server *server, struct session *session) {
    session_destroy(session);

    // Remove the session from the list
    if (session->prev != NULL) {
        session->prev->next = session->next;
    }
    if (session->next != NULL) {
        session->next->prev = session->prev;
    }
    if (server->sessions == session) {
        server->sessions = NULL;
    }
    server->num_sessions--;

    free(session);
}

bool server_next_session(struct server *server, struct session **session) {
    if (*session == NULL) {
        if (server->sessions == NULL) {
            return false;
        }
        *session = server->sessions;
    } else {
        if ((*session)->next == NULL) {
            return false;
        }
        *session = (*session)->next;
    }
    return true;
}
