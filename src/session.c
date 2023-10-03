#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "session.h"
#include "log.h"

_Atomic uint64_t next_session_id = 1;

bool session_create(struct session *session, int socket) {
    session->id = next_session_id++;

    session->socket = socket;

    session->state = malloc(sizeof(struct state));
    state_create(session->state);

    // Enable non-blocking mode
    u_long mode = 1;
    ioctl(session->socket, FIONBIO, &mode);

    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    if (getpeername(session->socket, (struct sockaddr *)&addr, &addr_size)) {
        LOG_ERROR("Failed to getpeername for session #%d: %s", session->id, strerror(errno));
    } else {
        char ip[20];
        strncpy(ip, inet_ntoa(addr.sin_addr), sizeof(ip));

        LOG_INFO("Session #%u created for %s:%d", session->id, ip, addr.sin_port);
    }

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
