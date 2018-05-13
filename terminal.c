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

void terminal_move(struct session *sess, unsigned x, unsigned y) {
    char buf[20];
    int len = snprintf(buf, 20, CSI "%d;%dH", y + 1, x + 1);
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

void terminal_rect(struct session *sess, unsigned x, unsigned y, unsigned w, unsigned h,
                   char symbol) {
    char *buf = malloc(sizeof(char) * w);
    memset(buf, symbol, w);

    terminal_move(sess, x, y);
    terminal_send(sess, buf, w);
    for (unsigned row = y; row < y + h - 1; ++row) {
        terminal_move(sess, x, row);
        terminal_send(sess, buf, 1);
        terminal_move(sess, x + w - 1, row);
        terminal_send(sess, buf, 1);
    }
    terminal_move(sess, x, y + h - 1);
    terminal_send(sess, buf, w);

    free(buf);
}

void terminal_write_at(struct session *sess, unsigned x, unsigned y, char *buf) {
    terminal_move(sess, x, y);
    terminal_write(sess, buf);
}

void terminal_reset(struct session *sess) {
    terminal_write(sess, CSI "m");
}

void terminal_bold(struct session *sess, bool state) {
    terminal_write(sess, state ? CSI "1m" : CSI "21m");
}

void terminal_underline(struct session *sess, bool state) {
    terminal_write(sess, state ? CSI "4m" : CSI "24m");
}

void terminal_inverse(struct session *sess, bool state) {
    terminal_write(sess, state ? CSI "7m" : CSI "27m");
}
