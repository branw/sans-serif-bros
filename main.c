#define _WIN32_WINNT _WIN32_WINNT_WIN8

#include <Winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdbool.h>

#define NUM_SESSIONS 10

struct session {
    bool valid;
    SOCKET client_sock;
};

void session_init(struct session *sess, SOCKET client_sock) {
    sess->valid = true;
    sess->client_sock = client_sock;

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
    unsigned char wont_echo[] = {0xff, 0xfe, 0x01};
    send(client_sock, (const char *) wont_echo, 3, 0);
    unsigned char will_sga[] = {0xff, 0xfb, 0x03};
    send(client_sock, (const char *) will_sga, 3, 0);
}

#define DEFAULT_BUFLEN 512

void session_update(struct session *sess) {
    char read_buf[DEFAULT_BUFLEN];
    int read_buf_len = DEFAULT_BUFLEN;

    int read_bytes = recv(sess->client_sock, read_buf, read_buf_len, 0);

    int err = WSAGetLastError();
    if (err && err != WSAEWOULDBLOCK) {
        fprintf(stderr, "recv failed (%d)\n", err);
        return;
    }

    if (read_bytes <= 0) return;


    for (int i = 0; i < read_bytes; ++i) {
        printf("%02x ", (unsigned char) (read_buf[i]));
    }
    printf("\n");
}

void session_close(struct session *sess) {
    int res = shutdown(sess->client_sock, SD_SEND);
    if (res == SOCKET_ERROR) {
        fprintf(stderr, "shutdown failed (%d)\n", WSAGetLastError());
    }

    closesocket(sess->client_sock);
    sess->valid = false;
}

struct session sessions[NUM_SESSIONS] = {0};
int next_session = 0;

SOCKET sock;

bool sock_init() {
    WSADATA wsa_data;

    int res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (res) {
        printf("WSAStartup failed (%d)\n", res);
        return false;
    }

    struct addrinfo *addr = NULL;
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    res = getaddrinfo(NULL, "4567", &hints, &addr);
    if (res) {
        fprintf(stderr, "getaddrinfo failed (%d)\n", res);
        return false;
    }

    sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "socket failed (%d)\n", WSAGetLastError());
        freeaddrinfo(addr);
        return false;
    }

    res = bind(sock, addr->ai_addr, (int) addr->ai_addrlen);
    if (res == SOCKET_ERROR) {
        fprintf(stderr, "bind failed (%d)\n", WSAGetLastError());
        freeaddrinfo(addr);
        closesocket(sock);
        return false;
    }

    freeaddrinfo(addr);

    res = listen(sock, SOMAXCONN);
    if (res == SOCKET_ERROR) {
        fprintf(stderr, "listen failed (%d)\n", WSAGetLastError());
        freeaddrinfo(addr);
        closesocket(sock);
        return false;
    }

    return true;
}

void sock_accept() {
    SOCKET client_sock = accept(sock, NULL, NULL);
    if (client_sock == INVALID_SOCKET) {
        fprintf(stderr, "accept failed (%d)\n", WSAGetLastError());
        closesocket(client_sock);
        return;
    }

    session_init(&sessions[next_session++], client_sock);
}

int main() {
    if (!sock_init()) {
        return 1;
    }

    printf("Hello, World!\n");

    while (sock) {
        struct fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(sock, &read_set);

        struct timeval timeout;
        timerclear(&timeout);

        if (select(FD_SETSIZE, &read_set, NULL, NULL, &timeout)) {
            printf("got connection!");
            sock_accept();
        }

        for (int i = 0; i < NUM_SESSIONS; ++i) {
            if (sessions[i].valid) session_update(&sessions[i]);
        }
    }

    return 0;
}
