#ifndef SSB_TERMINAL_H
#define SSB_TERMINAL_H

#include <stdbool.h>

#define ECHO "\x01"
#define SUPPRESS_GO_AHEAD "\x03"
#define NAWS "\x1f"

#define IAC  "\xff"
#define DONT "\xfe"
#define DO   "\xfd"
#define WONT "\xfc"
#define WILL "\xfb"
#define SB   "\xfa"
#define GA   "\xf9"
#define EL   "\xf8"
#define EC   "\xf7"
#define AYT  "\xf6"
#define AO   "\xf5"
#define IP   "\xf4"
#define BRK  "\xf3"
#define DM   "\xf2"
#define NOP  "\xf1"
#define SE   "\xf0"
#define EOR  "\xef"

struct session;

void terminal_send(struct session *sess, char *buf, int len);

void terminal_recv(struct session *sess, char *buf, int len);

void terminal_parse(struct session *sess, char *buf, int len);

unsigned terminal_available(struct session *sess);

char terminal_read(struct session *sess);

char terminal_peek(struct session *sess, unsigned n);

void terminal_write(struct session *sess, char *buf);

void terminal_move(struct session *sess, unsigned x, unsigned y);

void terminal_clear(struct session *sess);

void terminal_cursor(struct session *sess, bool state);

void terminal_rect(struct session *sess, unsigned x, unsigned y, unsigned w, unsigned h,
                   char symbol);

void terminal_write_at(struct session *sess, unsigned x, unsigned y, char *buf);

void terminal_reset(struct session *sess);

void terminal_bold(struct session *sess, bool state);

void terminal_underline(struct session *sess, bool state);

void terminal_inverse(struct session *sess, bool state);

// Number of bytes to store for each session; must be a power of 2
#define INPUT_BUF_LEN 512

struct terminal_state {
    char input_buf[INPUT_BUF_LEN];
    unsigned input_buf_read, input_buf_write;
};

void terminal_init(struct terminal_state *state);

struct menu_input {
    bool esc, enter, space;
    int x, y;
};

void terminal_read_menu_input(struct session *sess, struct menu_input *input);

#endif //SSB_TERMINAL_H
