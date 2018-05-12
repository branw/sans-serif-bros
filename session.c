#define _WIN32_WINNT _WIN32_WINNT_WIN8

#include <ws2tcpip.h>
#include <stdio.h>

#include "session.h"

bool session_create(struct session *session, SOCKET client_sock) {
    session->client_sock = client_sock;
    session->last_tick.tv_sec = session->last_tick.tv_nsec = 0;

    SOCKADDR_IN info = {0};
    int info_size = sizeof(info);
    getpeername(client_sock, (struct sockaddr *) &info, &info_size);

    char ip[16];
    inet_ntop(AF_INET, &info.sin_addr, ip, 16);

    printf("Client %s connected\n", ip);

    u_long mode = 1;
    ioctlsocket(client_sock, FIONBIO, &mode);

    unsigned char will_echo[] = {0xff, 0xfb, 0x01};
    send(client_sock, (const char *) will_echo, 3, 0);
    unsigned char will_sga[] = {0xff, 0xfb, 0x03};
    send(client_sock, (const char *) will_sga, 3, 0);
}

void session_shutdown(struct session *sess) {
    SOCKADDR_IN info = {0};
    int info_size = sizeof(info);
    getpeername(sess->client_sock, (struct sockaddr *) &info, &info_size);

    char ip[16];
    inet_ntop(AF_INET, &info.sin_addr, ip, 16);

    printf("Client %s disconnected\n", ip);

    int res = shutdown(sess->client_sock, SD_SEND);
    if (res == SOCKET_ERROR) {
        fprintf(stderr, "shutdown failed (%d)\n", WSAGetLastError());
    }

    closesocket(sess->client_sock);
}

#define RECV_BUF_LEN 512

bool session_update(struct session *sess) {
    char recv_buf[RECV_BUF_LEN];

    // Receive a block from the client
    int recv_len = recv(sess->client_sock, recv_buf, RECV_BUF_LEN, 0);
    if (recv_len == SOCKET_ERROR) {
        int err = WSAGetLastError();
        // There's nothing to receive
        if (err == WSAEWOULDBLOCK) {
            return true;
        }
            // An actual error occurred
        else {
            fprintf(stderr, "recv failed (%d)\n", err);
            return false;
        }
    }

    // The client disconnected
    if (recv_len == 0) {
        return false;
    }

    for (int i = 0; i < recv_len; ++i) {
        printf("%02x ", (unsigned char) (recv_buf[i]));
    }
    printf("\n");

    return true;
}
