#ifdef _WIN32
#define _WIN32_WINNT _WIN32_WINNT_WIN8

#include <ws2tcpip.h>

#define ERR_NUM (WSAGetLastError())
#define GET_ERR_MSG(msg) char (msg)[256]; FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | \
    FORMAT_MESSAGE_IGNORE_INSERTS, NULL, ERR_NUM, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
    (msg), 256, NULL)
#endif

#ifdef __linux
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define SOCKET_ERROR -1
#define INVALID_SOCKET -1

#define WSAEWOULDBLOCK EWOULDBLOCK

static int WSAGetLastError() {
    return errno;
}

#define ERR_NUM errno
#define GET_ERR_MSG(msg) char *(msg) = strerror(errno)
#endif

#include <stdio.h>

#include "session.h"

bool session_create(struct session *session, SOCKET socket) {
    session->socket = socket;

    session->state = malloc(sizeof(struct state));
    state_create(session->state);

    // Enable non-blocking mode
    u_long mode = 1;
#if defined(_WIN32)
    ioctlsocket(session->socket, FIONBIO, &mode);
#elif defined(__linux__) || defined(__APPLE__)
    ioctl(session->socket, FIONBIO, &mode);
#endif

    return true;
}

void session_destroy(struct session *session) {
    state_destroy(session->state);
    free(session->state);

    // Prevent anymore sending on the socket
#if defined(_WIN32)
    int result = shutdown(session->socket, SD_SEND);
#elif defined(__linux__) || defined(__APPLE__)
    int result = shutdown(session->socket, SHUT_WR);
#endif
    if (result == -1) {
        GET_ERR_MSG(err_msg);
        fprintf(stderr, "shutdown failed (%d: %s)\n", ERR_NUM, err_msg);
    }

    // Close the socket
#if defined(_WIN32)
    closesocket(session->socket);
#elif defined(__linux__) || defined(__APPLE__)
    close(session->socket);
#endif
}

bool session_receive(struct session *session, char *buf, size_t len, size_t *len_written) {
    ssize_t recv_len = recv(session->socket, buf, (int) len, 0);
    // An error occurred while receiving
    if (recv_len == -1) {
        // Nothing was transmitted since the last receive
#if defined(_WIN32)
        if (ERR_NUM == WSAEWOULDBLOCK) {
#elif defined(__linux__) || defined(__APPLE__)
        if (ERR_NUM == EWOULDBLOCK) {
#endif
            *len_written = 0;
            return true;
        }

        GET_ERR_MSG(err_msg);
        fprintf(stderr, "recv failed (%d: %s)\n", ERR_NUM, err_msg);
        return false;
    }
    // The client disconnected
    else if (recv_len == 0) {
        return false;
    }

    printf("-> ");
    for (ssize_t i = 0; i < recv_len; ++i) {
        printf("%02x ", (unsigned char) buf[i]);
    }
    printf("\n");

    *len_written = (size_t) recv_len;
    return true;
}

void session_send(struct session *session, char *buf, size_t len) {
    if (send(session->socket, buf, (int) len, 0) == -1) {
        GET_ERR_MSG(err_msg);
        fprintf(stderr, "send failed (%d: %s)\n", ERR_NUM, err_msg);
    }

    printf("<- ");
    for (int i = 0; i < len; ++i) {
        printf("%02x ", (unsigned char) buf[i]);
    }
    printf("\n");
}
