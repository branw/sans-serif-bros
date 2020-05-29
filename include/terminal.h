#ifndef SSB_TERMINAL_H
#define SSB_TERMINAL_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define KEYBOARD_KEY_PRESSED(input, key) \
    (key >= '0' && key <= '9' ? \
        ((input).nums & (1 << ((key) - '0'))) == (1 << ((key) - '0')) : \
        ((input).alphas & (1 << (((key) & 0xdf) - 'A'))) == (1 << (((key) & 0xdf) - 'A')))

#define KEYBOARD_F_KEY_PRESSED(input, key) \
    ((input).fs & (1 << (key)) == (1 << (key)))

#define KEYBOARD_CLEAR(input) \
    memset(&input, 0, sizeof(struct keyboard_input))

struct directional_input {
    int up : 1;
    int down : 1;
    int left : 1;
    int right : 1;
};

struct keyboard_input {
    uint32_t alphas;
    uint16_t nums;
    uint16_t fs;

    int esc : 1;
    int tab : 1;
    int backspace : 1;
    int enter : 1;
    int space : 1;

    int up : 1;
    int down : 1;
    int left : 1;
    int right : 1;

    int insert : 1;
    int delete : 1;
    int home : 1;
    int page_up : 1;
    int page_down : 1;
};

struct terminal {
    struct canvas *canvas;

    struct keyboard_input keyboard;

    char buffer[4096];
    int buffer_len, buffer_flushed_len;
};

// Create a new terminal instance
bool terminal_create(struct terminal *terminal, struct canvas *canvas);

// Destroy an existing terminal instance
void terminal_destroy(struct terminal *terminal);

void terminal_parse(struct terminal *terminal, char *buf, size_t len);

bool terminal_flush(struct terminal *terminal, char *buf, size_t len,
                    size_t *len_written);

void terminal_write_bytes(struct terminal *terminal, char *buf, size_t len);

void terminal_write(struct terminal *terminal, char *buf);


void terminal_reset(struct terminal *terminal);

void terminal_clear(struct terminal *terminal);

void terminal_move(struct terminal *terminal, unsigned x, unsigned y);

void terminal_cursor(struct terminal *terminal, bool state);

void terminal_get_directional_input(struct terminal *terminal, struct directional_input *input, bool wasd);

/*
struct session;

// Number of bytes to store for each session; must be a power of 2
#define INPUT_BUF_LEN 512

struct terminal_state {
    char input_buf[INPUT_BUF_LEN];
    unsigned input_buf_read, input_buf_write;

    bool dimensions_reported;
    unsigned w, h;
};

void terminal_init(struct terminal_state *state);

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

void terminal_pretty_rect(struct session *sess, unsigned x, unsigned y, unsigned w, unsigned h);

void terminal_reset(struct session *sess);

void terminal_bold(struct session *sess, bool state);

void terminal_underline(struct session *sess, bool state);

void terminal_inverse(struct session *sess, bool state);

struct menu_input {
    bool esc, enter, space;
    int x, y;
};

void terminal_read_menu_input(struct session *sess, struct menu_input *input);

bool terminal_dimensions(struct session *sess, unsigned *w, unsigned *h);
*/

#endif //SSB_TERMINAL_H
