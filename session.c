#define _WIN32_WINNT _WIN32_WINNT_WIN8

#include <ws2tcpip.h>
#include <stdio.h>

#include "session.h"

bool session_create(struct session *sess, SOCKET client_sock) {
    sess->client_sock = client_sock;

    SOCKADDR_IN info = {0};
    int info_size = sizeof(info);
    getpeername(client_sock, (struct sockaddr *) &info, &info_size);

    // Get client IP address
    char ip[16];
    inet_ntop(AF_INET, &info.sin_addr, ip, 16);

    printf("#%d connected (%s)\n", sess->id, ip);

    // Use non-blocking mode
    u_long mode = 1;
    ioctlsocket(client_sock, FIONBIO, &mode);

    // Initialize the game state
    state_init(sess);
}

void session_shutdown(struct session *sess) {
    printf("#%d disconnected\n", sess->id);

    // Disable sending to the socket
    int res = shutdown(sess->client_sock, SD_SEND);
    if (res == SOCKET_ERROR) {
        fprintf(stderr, "#%d: shutdown failed (%d)\n", sess->id, WSAGetLastError());
    }

    closesocket(sess->client_sock);
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
