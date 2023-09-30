#include <string.h>
#include <stdio.h>
#include <baro.h>
#include "terminal.h"
#include "session.h"
#include "telnet.h"
#include "util.h"

bool terminal_create(struct terminal *terminal, struct canvas *canvas) {
    terminal->canvas = canvas;

    terminal->buffer_len = terminal->buffer_flushed_len = 0;

    terminal->keyboard = (struct keyboard_input){0};

    terminal->will_naws = false;

    return true;
}

void terminal_destroy(struct terminal *terminal) {

}

// Parse a 16-bit value from the buffer. This obviously requires at least
// 2 bytes, but may require up to 4 as 0xFF/IAC must be escaped.
static bool parse_escaped_u16(char **buf, size_t *len, uint16_t *out) {
    if (*len < 2) {
        return false;
    }

    char *working_buf = *buf;
    size_t working_len = *len;

    if (working_buf[0] == '\xff') {
        // 0xFF only makes sense when it's doubled/escaped
        if (working_len < 3 || working_buf[1] != '\xff') {
            return false;
        }
        working_buf += 1;
        working_len -= 1;
    }

    uint16_t result = (uint8_t)(working_buf[0]) << 8;
    working_buf += 1;
    working_len -= 1;

    if (working_buf[0] == '\xff') {
        if (working_len < 2 || working_buf[1] != '\xff') {
            return false;
        }
        working_buf += 1;
        working_len -= 1;
    }

    result |= (uint8_t)(working_buf[0]);
    working_buf += 1;
    working_len -= 1;

    *buf = working_buf;
    *len = working_len;
    *out = result;
    return true;
}

TEST("[terminal] parse_escaped_u16") {
    {
        uint8_t data[2] = {0x43, 0x21};

        char *buf = (char *) data;
        size_t len = 2;
        uint16_t out = 0x1337;
        REQUIRE(parse_escaped_u16(&buf, &len, &out));
        REQUIRE_EQ(buf, &data[2]);
        REQUIRE_EQ(len, 0);
        REQUIRE_EQ(out, 0x4321);
    }

    // A single/un-escaped 0xFF is invalid
    {
        uint8_t data[3] = {0xff, 0x12, 0x34};

        char *buf = (char *)data;
        size_t len = 3;
        uint16_t out = 0x1337;
        REQUIRE_FALSE(parse_escaped_u16(&buf, &len, &out));
    }

    {
        uint8_t data[4] = {0xff, 0xff, 0xff, 0xff};

        char *buf = (char *) data;
        size_t len = 4;
        uint16_t out = 0x1337;
        REQUIRE(parse_escaped_u16(&buf, &len, &out));
        REQUIRE_EQ(buf, &data[4]);
        REQUIRE_EQ(len, 0);
        REQUIRE_EQ(out, 0xffff);
    }

    {
        uint8_t data[3] = {0x12, 0xff, 0xff};

        char *buf = (char *) data;
        size_t len = 3;
        uint16_t out = 0x1337;
        REQUIRE(parse_escaped_u16(&buf, &len, &out));
        REQUIRE_EQ(buf, &data[3]);
        REQUIRE_EQ(len, 0);
        REQUIRE_EQ(out, 0x12ff);
    }
}

static bool parse_telnet_command(struct terminal *terminal, char **buf, size_t *len) {
    // Escaped chr 255, which we don't care about
    if ((*buf)[1] == *IAC) {
        *buf += 2;
        *len -= 2;
        return true;
    }

    // Negotiations:
    // IAC WILL/WONT/DO/DONT <option>
    if (*len >= 3 &&
        ((*buf)[1] == *WILL || (*buf)[1] == *WONT ||
                (*buf)[1] == *DO || (*buf)[1] == *DONT)) {
        // "Negotiate About Window Size"
        if ((*buf)[2] == *NAWS) {
            terminal->will_naws = ((*buf)[1] == *WILL);
        }

        printf("Received Telnet negotiation: %s <%u>\n",
               (*buf)[1] == *WILL ? "WILL" :
               (*buf)[1] == *WONT ? "WONT" :
               (*buf)[1] == *DO ? "DO" :
               (*buf)[1] == *DONT ? "DONT" : "???",
               (uint8_t)((*buf)[2]));

        *buf += 3;
        *len -= 3;
        return true;
    }

    // Sub-options:
    // IAC SB <option> <values...> IAC SE
    if (*len >= 5 && (*buf)[1] == *SB) {
        if ((*buf)[2] == *NAWS) {
            char *original_buf = *buf;
            size_t original_len = *len;
            *buf += 3;
            *len -= 3;

            uint16_t cols = 0, rows = 0;
            if (!parse_escaped_u16(buf, len, &cols) ||
                    !parse_escaped_u16(buf, len, &rows) ||
                    (*len >= 2 && (*buf)[0] != *IAC && (*buf)[1] != *SE)) {
                *buf = original_buf;
                *len = original_len;
                return false;
            }

            printf("Received NAWS: %d cols, %d rows\n", cols, rows);
            uint16_t clamped_cols = SSB_CLAMP(cols, COLUMNS, 200);
            uint16_t clamped_rows = SSB_CLAMP(rows, ROWS, 200);
            if (clamped_cols != cols || clamped_rows != rows) {
                fprintf(stderr, "Clamping NAWS resolution (%d cols, %d rows) to %d cols, %d rows",
                        cols, rows, clamped_cols, clamped_rows);
            }

            canvas_resize(terminal->canvas, clamped_cols, clamped_rows);

            return true;
        }
    }

    return false;
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
        else if (ch == ' ') {
            terminal->keyboard.space = 1;
        }
        else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
            unsigned place = (ch & 0xdf) - 'A';
            terminal->keyboard.alphas |= 1u << place;
        }
        else if (ch >= '0' && ch <= '9') {
            unsigned place = ch - '0';
            terminal->keyboard.nums |= 1u << place;
        }
        else if (ch == '\x0d') {
            terminal->keyboard.enter = 1;
        }
        // This could also be triggered by unhandled escape sequences
        else if (ch == '\x1b') {
            terminal->keyboard.esc = 1;
        }
        // Telnet command
        else if (ch == *IAC && len >= 2) {
            if (parse_telnet_command(terminal, &buf, &len)) {
                continue;
            }
        }

        buf++;
        len--;
    }
}

bool terminal_flush(struct terminal *terminal, char *buf, size_t len, size_t *len_written) {
    if (terminal->buffer_flushed_len != terminal->buffer_len) {
        size_t to_write = terminal->buffer_len - terminal->buffer_flushed_len;
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
