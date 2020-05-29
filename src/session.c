#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "session.h"

bool session_create(struct session *session, int socket) {
    session->socket = socket;

    session->state = malloc(sizeof(struct state));
    state_create(session->state);

    // Enable non-blocking mode
    u_long mode = 1;
    ioctl(session->socket, FIONBIO, &mode);

    return true;
}

void session_destroy(struct session *session) {
    state_destroy(session->state);
    free(session->state);

    // Prevent anymore sending on the socket
    int result = shutdown(session->socket, SHUT_WR);
    if (result == -1) {
        fprintf(stderr, "shutdown failed (%d: %s)\n", errno, strerror(errno));
    }

    // Close the socket
    close(session->socket);
}

bool session_receive(struct session *session, char *buf, size_t len, size_t *len_written) {
    ssize_t recv_len = recv(session->socket, buf, (int) len, 0);
    // An error occurred while receiving
    if (recv_len == -1) {
        // Nothing was transmitted since the last receive
        if (errno == EWOULDBLOCK) {
            *len_written = 0;
            return true;
        }

        fprintf(stderr, "recv failed (%d: %s)\n", errno, strerror(errno));
        return false;
    }
    // The client disconnected
    else if (recv_len == 0) {
        return false;
    }

    printf("-> ");
    for (ssize_t i = 0; i < recv_len; ++i) {
        if (buf[i] >= 0x20 && buf[i] <= 0x7e) printf("%c", buf[i]);
        else printf("\\x%02x", (unsigned char) buf[i]);
    }
    printf("\n");

    *len_written = (size_t) recv_len;
    return true;
}

void session_send(struct session *session, char *buf, size_t len) {
    if (send(session->socket, buf, (int) len, 0) == -1) {
        fprintf(stderr, "send failed (%d: %s)\n", errno, strerror(errno));
    }

    printf("<- ");
    for (int i = 0; i < len; ++i) {
        if (buf[i] >= 0x20 && buf[i] <= 0x7e) printf("%c", buf[i]);
        else printf("\\x%02x", (unsigned char) buf[i]);
    }
    printf("\n");
}
