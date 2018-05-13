#include <winsock2.h>
#include <stdio.h>
#include "terminal.h"
#include "session.h"
#include "config.h"

void terminal_send(struct session *sess, char *buf, int len) {
#if DEBUG_OUTGOING
    printf("#%d <- ", sess->id);
    for (int i = 0; i < len; ++i) {
        printf("%02x ", (unsigned char) buf[i]);
    }
    printf("\n");
#endif

    send(sess->client_sock, buf, len, 0);
}

void terminal_recv(struct session *sess, char *buf, int len) {
#if DEBUG_INCOMING
    printf("#%d -> ", sess->id);
    for (int i = 0; i < len; ++i) {
        printf("%02x ", (unsigned char) buf[i]);
    }
    printf("\n");
#endif

    terminal_parse(sess, buf, len);
}

void terminal_parse(struct session *sess, char *buf, int len) {
    //TODO strip NVT packets
}

void terminal_write(struct session *sess, char *buf) {
    terminal_send(sess, buf, (int) strlen(buf));
}

#define ESC "\e"
#define CSI ESC "["

void terminal_move(struct session *sess, int x, int y) {
    char buf[10];
    int len = snprintf(buf, 10, CSI "%d;%dH", x, y);
    terminal_send(sess, buf, len);
}

void terminal_clear(struct session *sess) {
    char buf[] = CSI "2J";
    terminal_send(sess, buf, 4);
}

void terminal_cursor(struct session *sess, bool state) {
    char buf[10];
    int len = snprintf(buf, 10, CSI "?25%c", (state ? 'h' : 'l'));
    terminal_send(sess, buf, len);
}
