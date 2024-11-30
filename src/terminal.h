#ifndef SSB_TERMINAL_H
#define SSB_TERMINAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define KEYBOARD_KEY_PRESSED(input, key) \
    (key >= '0' && key <= '9' ? \
        ((input).nums & (1ul << ((key) - '0'))) == (1ul << ((key) - '0')) : \
        ((input).alphas & (1ul << (((key) & 0xdf) - 'A'))) == (1ul << (((key) & 0xdf) - 'A')))

#define KEYBOARD_F_KEY_PRESSED(input, key) \
    ((input).fs & (1 << (key)) == (1 << (key)))

#define KEYBOARD_CLEAR(input) \
    (input) = (struct keyboard_input){0}

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

    uint8_t esc : 1;
//    uint8_t tab : 1;
//    uint8_t backspace : 1;
    uint8_t enter : 1;
    uint8_t space : 1;

    uint8_t up : 1;
    uint8_t down : 1;
    uint8_t left : 1;
    uint8_t right : 1;

//    uint8_t insert : 1;
//    uint8_t del : 1;
//    uint8_t home : 1;
//    uint8_t page_up : 1;
//    uint8_t page_down : 1;
};

struct terminal {
    struct canvas *canvas;

    struct keyboard_input keyboard;

    char buffer[4096];
    size_t buffer_len, buffer_flushed_len;

    bool will_naws;
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

#ifdef __cplusplus
}
#endif

#endif //SSB_TERMINAL_H
