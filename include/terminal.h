#ifndef SSB_TERMINAL_H
#define SSB_TERMINAL_H

#include <stdbool.h>


struct menu_input {
    bool esc, enter, space;
    int x, y;
};

struct terminal {
    struct canvas *canvas;

    struct menu_input input;

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
