#include <stdio.h>
#include "state.h"
#include "session.h"

void state_init(struct session *sess) {
    terminal_send(sess, IAC WILL ECHO, 3);
    terminal_send(sess, IAC DONT ECHO, 3);
    terminal_send(sess, IAC WILL SUPPRESS_GO_AHEAD, 3);

    terminal_write(sess, "Super Serif Bros. Telnet Edition\n\r"
                         "(commit " GIT_COMMIT_HASH " from " GIT_COMMIT_TIMESTAMP ")\n\r");

    terminal_clear(sess);
    terminal_reset(sess);
    terminal_cursor(sess, false);

    sess->state.screen = title_screen;
    clock_gettime(CLOCK_MONOTONIC, &sess->state.last_tick);
    sess->state.input_buf_read = sess->state.input_buf_write = 0;
}

struct menu_input {
    bool esc, enter, space;
    int x, y;
};

static void parse_menu_input(struct session *sess, struct menu_input *input) {
    input->esc = input->enter = input->space = false;
    input->x = input->y = 0;

    int available;
    while ((available = terminal_available(sess)) > 0) {
        char val0 = terminal_read(sess), val1 = terminal_peek(sess, 0),
                val2 = terminal_peek(sess, 1);
        switch (available) {
        default:
        case 3:
            if (val0 == '\x1b' && val1 == '[' && val2 == 'A') {
                input->y = 1;
            }
            if (val0 == '\x1b' && val1 == '[' && val2 == 'B') {
                input->y = -1;
            }
            if (val0 == '\x1b' && val1 == '[' && val2 == 'C') {
                input->x = 1;
            }
            if (val0 == '\x1b' && val1 == '[' && val2 == 'D') {
                input->x = -1;
            }

        case 2:
            if (val0 == '\x0d' && (val1 == '\x00' || val1 == '\x0a')) {
                input->enter = true;
            }

        case 1:
            if (val0 == '\x1b' && available == 1) {
                input->esc = true;
            }
            if (val0 == '\x20') {
                input->space = true;
            }
        }
    }
}

static bool title_screen_update(struct session *sess) {
    terminal_move(sess, 0, 0);
    terminal_rect(sess, 0, 0, 80, 25, '#');

    // Parse input buffer
    struct menu_input input;
    parse_menu_input(sess, &input);

    if (input.esc) {
        return false;
    }

    if (input.space || input.enter) {
        terminal_inverse(sess, true);
    } else {
        terminal_inverse(sess, false);
    }

    static unsigned x = 39, y = 12;

    x += input.x;
    y -= input.y;

    terminal_move(sess, x, y);
    terminal_write(sess, "@");

    terminal_inverse(sess, false);

    return true;
}

// Convert timespec to number in milliseconds
#define TO_MS(t) ((t).tv_sec * 1000 + (t).tv_nsec / 1000000)

bool state_update(struct session *sess) {
    struct timespec current;
    clock_gettime(CLOCK_MONOTONIC, &current);

    // Check if a tick has elapsed
    long long delta = TO_MS(current) - TO_MS(sess->state.last_tick);
    if (delta < TICK_DURATION) {
        return true;
    }

    sess->state.last_tick = current;

    // Dispatch state handler
    switch (sess->state.screen) {
    default:
    case title_screen:
        return title_screen_update(sess);
    }
}
