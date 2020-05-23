#ifdef _WIN32
#define _WIN32_WINNT _WIN32_WINNT_WIN8

#include <ws2tcpip.h>

#endif

#ifdef __linux
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>

#define SOCKET_ERROR -1
#define INVALID_SOCKET -1

#define WSAEWOULDBLOCK EWOULDBLOCK

static int WSAGetLastError() {
    return errno;
}
#endif

#include <unistd.h>
#include <stdio.h>

#include "session.h"

bool session_create(struct session *sess, SOCKET client_sock) {
    sess->client_sock = client_sock;

    struct sockaddr_in info = {0};
    unsigned info_size = sizeof(info);
    getpeername(client_sock, (struct sockaddr *) &info, &info_size);

    // Get client IP address
    char ip[16];
    inet_ntop(AF_INET, &info.sin_addr, ip, 16);

    printf("#%d connected (%s)\n", sess->id, ip);

    // Use non-blocking mode
    u_long mode = 1;
#ifdef _WIN32
    ioctlsocket(client_sock, FIONBIO, &mode);
#endif
#ifdef __linux__
    ioctl(client_sock, FIONBIO, &mode);
#endif

    // Initialize the game state
    state_init(sess);

    return true;
}

void session_shutdown(struct session *sess) {
    printf("#%d disconnected\n", sess->id);

    // Disable sending to the socket
#ifdef _WIN32
    int res = shutdown(sess->client_sock, SD_SEND);
#endif
#ifdef __linux__
    int res = shutdown(sess->client_sock, SHUT_WR);
#endif
    if (res == SOCKET_ERROR) {
        fprintf(stderr, "#%d: shutdown failed (%d)\n", sess->id, WSAGetLastError());
    }

#ifdef _WIN32
    closesocket(sess->client_sock);
#endif
#ifdef __linux
    close(sess->client_sock);
#endif
}

#define RECV_BUF_LEN 512

static bool handle_input(struct session *sess) {
    char recv_buf[RECV_BUF_LEN];

    // Receive a packet from the client
    int recv_len = recv(sess->client_sock, recv_buf, RECV_BUF_LEN, 0);
    if (recv_len == SOCKET_ERROR) {
        int err = WSAGetLastError();
        // There's nothing to receive
        if (err == WSAEWOULDBLOCK) {
            return true;
        }
            // An actual error occurred
        else {
            fprintf(stderr, "#%d: recv failed (%d)\n", sess->id, err);
            return false;
        }
    }

    // The client disconnected
    if (recv_len == 0) {
        return false;
    }

    // Parse the input
    terminal_recv(sess, recv_buf, recv_len);

    return true;
}

bool session_update(struct session *sess) {
    // Process input and execute the current state
    return handle_input(sess) && state_update(sess);
}
