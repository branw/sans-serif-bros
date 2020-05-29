#include <string.h>
#include <stdio.h>
#include "terminal.h"
#include "session.h"
#include "telnet.h"

bool terminal_create(struct terminal *terminal, struct canvas *canvas) {
    terminal->canvas = canvas;

    terminal->buffer_len = terminal->buffer_flushed_len = 0;

    return true;
}

void terminal_destroy(struct terminal *terminal) {

}

void terminal_parse(struct terminal *terminal, char *buf, size_t len) {
    while (len) {
        char ch = *buf;

        // Escape sequences
        if (ch == '\x1b' && len >= 2) {
            // CSI sequences
            if (buf[1] == '[' && len >= 3) {
                // Move cursor
                if (buf[2] >= 'A' && buf[2] <= 'D') {
                   if (buf[2] == 'A') terminal->keyboard.up = 1;
                   else if (buf[2] == 'B') terminal->keyboard.down = 1;
                   else if (buf[2] == 'C') terminal->keyboard.right = 1;
                   else if (buf[2] == 'D') terminal->keyboard.left = 1;

                    buf += 3;
                    len -= 3;
                    continue;
                }
            }
        }

        buf++;
        len--;
    }
}

bool terminal_flush(struct terminal *terminal, char *buf, size_t len, size_t *len_written) {
    if (terminal->buffer_flushed_len != terminal->buffer_len) {
        int to_write = terminal->buffer_len - terminal->buffer_flushed_len;
        if (to_write > len) {
            to_write = len;
        }

        memcpy(buf, &terminal->buffer[terminal->buffer_flushed_len], to_write);

        terminal->buffer_flushed_len += to_write;

        if (terminal->buffer_flushed_len == terminal->buffer_len) {
            terminal->buffer_len = terminal->buffer_flushed_len = 0;
        }

        *len_written = to_write;

        return true;
    }

    return canvas_flush(terminal->canvas, buf, len, len_written);
}

void terminal_write_bytes(struct terminal *terminal, char *buf, size_t len) {
    if (terminal->buffer_len + len >= 4096) {
        return;
    }

    memcpy(&terminal->buffer[terminal->buffer_len], buf, len);

    terminal->buffer_len += len;
}

void terminal_write(struct terminal *terminal, char *buf) {
    terminal_write_bytes(terminal, buf, strlen(buf));
}

void terminal_reset(struct terminal *terminal) {
    terminal_write(terminal, CSI "m");
}

void terminal_clear(struct terminal *terminal) {
    terminal_write(terminal, CSI "2J");
}

void terminal_move(struct terminal *terminal, unsigned x, unsigned y) {
    char buf[20];
    int len = snprintf(buf, 20, CSI "%d;%dH", y + 1, x + 1);
    terminal_write_bytes(terminal, buf, len);
}

void terminal_cursor(struct terminal *terminal, bool state) {
    terminal_write(terminal, state ? CSI "?25h" : CSI "?25l");
}

void terminal_get_directional_input(struct terminal *terminal,
        struct directional_input *input, bool wasd) {
    *input = (struct directional_input){0};

    if (wasd) {
        input->up = KEYBOARD_KEY_PRESSED(terminal->keyboard, 'W');
        input->down = KEYBOARD_KEY_PRESSED(terminal->keyboard, 'S');
        input->left = KEYBOARD_KEY_PRESSED(terminal->keyboard, 'A');
        input->right = KEYBOARD_KEY_PRESSED(terminal->keyboard, 'D');
    }
    else {
        input->up = terminal->keyboard.up;
        input->down = terminal->keyboard.down;
        input->left = terminal->keyboard.left;
        input->right = terminal->keyboard.right;
    }
}
