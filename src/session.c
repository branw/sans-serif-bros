#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "session.h"
#include "log.h"

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
        LOG_ERROR("shutdown failed (%d: %s)", errno, strerror(errno));
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

        LOG_ERROR("recv failed (%d: %s)", errno, strerror(errno));
        return false;
    }
    // The client disconnected
    else if (recv_len == 0) {
        return false;
    }

    *len_written = (size_t) recv_len;

    if (recv_len > 4095) {
        LOG_WARN("Skipping logging received packet because it was too long (%d bytes)", recv_len);
        return true;
    }

    char buffer[4096];
    int written_len = snprintf(buffer, 4096, "-> ");
    for (int i = 0; i < recv_len; ++i) {
        if (buf[i] >= 0x20 && buf[i] <= 0x7e) {
            written_len += snprintf(buffer + written_len, 4096 - written_len, "%c", buf[i]);
        }else {
            written_len += snprintf(buffer + written_len, 4096 - written_len, "\\x%02x", (uint8_t) buf[i]);
        }
    }
    LOG_TRACE(buffer);

    return true;
}

void session_send(struct session *session, char *buf, size_t len) {
    if (send(session->socket, buf, (int) len, 0) == -1) {
        LOG_ERROR("send failed (%d: %s)", errno, strerror(errno));
        return;
    }

    if (len > 4095) {
        LOG_WARN("Skipping logging sent packet because it was too long (%d bytes)", len);
        return;
    }

    char buffer[4096];
    int written_len = snprintf(buffer, 4096, "<- ");
    for (int i = 0; i < len; ++i) {
        if (buf[i] >= 0x20 && buf[i] <= 0x7e) {
            written_len += snprintf(buffer + written_len, 4096 - written_len, "%c", buf[i]);
        }else {
            written_len += snprintf(buffer + written_len, 4096 - written_len, "\\x%02x", (uint8_t) buf[i]);
        }
    }
    LOG_TRACE(buffer);
}
