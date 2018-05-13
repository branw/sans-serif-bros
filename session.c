#define _WIN32_WINNT _WIN32_WINNT_WIN8

#include <ws2tcpip.h>
#include <stdio.h>

#include "session.h"

bool session_create(struct session *sess, SOCKET client_sock) {
    sess->client_sock = client_sock;

    SOCKADDR_IN info = {0};
    int info_size = sizeof(info);
    getpeername(client_sock, (struct sockaddr *) &info, &info_size);

    char ip[16];
    inet_ntop(AF_INET, &info.sin_addr, ip, 16);

    printf("#%d connected (%s)\n", sess->id, ip);

    u_long mode = 1;
    ioctlsocket(client_sock, FIONBIO, &mode);

    unsigned char will_echo[] = {0xff, 0xfb, 0x01};
    send(client_sock, (const char *) will_echo, 3, 0);
    unsigned char will_sga[] = {0xff, 0xfb, 0x03};
    send(client_sock, (const char *) will_sga, 3, 0);
    unsigned char do_naws[] = {0xff, 253, 31};
    send(client_sock, (const char *) do_naws, 3, 0);

    terminal_write(sess, "Super Serif Bros. Telnet Edition\n\r"
                         "(commit " GIT_COMMIT_HASH " from " GIT_COMMIT_TIMESTAMP ")\n\r");
}

void session_shutdown(struct session *sess) {
    printf("#%d disconnected\n", sess->id);

    int res = shutdown(sess->client_sock, SD_SEND);
    if (res == SOCKET_ERROR) {
        fprintf(stderr, "shutdown failed (%d)\n", WSAGetLastError());
    }

    closesocket(sess->client_sock);
}

#define RECV_BUF_LEN 512

static bool update_input(struct session *sess) {
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
            fprintf(stderr, "recv failed (%d)\n", err);
            return false;
        }
    }

    // The client disconnected
    if (recv_len == 0) {
        return false;
    }

    printf("#%d: ", sess->id);
    for (int i = 0; i < recv_len; ++i) {
        printf("%02x ", (unsigned char) recv_buf[i]);
    }
    printf("\n");

    terminal_parse(sess, recv_buf, recv_len);

    return true;
}

bool session_update(struct session *sess) {
    return update_input(sess) && state_update(sess);
}
