#include <winsock2.h>
#include <stdio.h>
#include "terminal.h"
#include "session.h"

#define IAC  '\xff'
#define DONT '\xfe'
#define DO   '\xfd'
#define WONT '\xfc'
#define WILL '\xfb'
#define SB   '\xfa'
#define GA   '\xf9'
#define EL   '\xf8'
#define EC   '\xf7'
#define AYT  '\xf6'
#define AO   '\xf5'
#define IP   '\xf4'
#define BRK  '\xf3'
#define DM   '\xf2'
#define NOP  '\xf1'
#define SE   '\xf0'
#define EOR  '\xef'

void terminal_parse(struct session *sess, char *buf, int len) {
    for (int i = 0; i < len; ++i) {
        switch (sess->state.parse_state) {
            // parse_data, parse_iac, parse_will, parse_wont, parse_do, parse_dont, parse_sb, parse_sb_data,
            //    parse_sb_data_iac
        default:
        case parse_data:
            switch (buf[i]) {
            case IAC:
                sess->state.parse_state = parse_iac;
                break;

            default:
                //TODO
                break;
            }
            break;

        case parse_iac:

            break;

        case parse_will:
            break;

        case parse_wont:
            break;

        case parse_do:
            break;

        case parse_dont:
            break;

        case parse_sb:

            break;

        case parse_sb_data:
            switch (buf[i]) {
            case IAC:
                sess->state.parse_state = parse_sb_data_iac;
                break;

            default:
                //TODO
                break;
            }
            break;

        case parse_sb_data_iac:
            switch (buf[i]) {
            case SE:
                sess->state.parse_state = parse_data;
                break;

            default:
                //TODO
                sess->state.parse_state = parse_sb_data;
            }
            break;
        }
    }
}

void terminal_write(struct session *sess, char *buf) {
    send(sess->client_sock, buf, (int) strlen(buf), 0);
}

#define ESC "\x1b"

void terminal_move(struct session *sess, int x, int y) {
    char buf[10];
    int len = snprintf(buf, 10, ESC "[%d;%dH", x, y);
    send(sess->client_sock, buf, len, 0);
}
