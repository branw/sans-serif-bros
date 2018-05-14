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

static bool title_screen_update(struct session *sess) {
    terminal_move(sess, 0, 0);
    terminal_rect(sess, 0, 0, 80, 25, '#');

    terminal_move(sess, 25, 11);
    terminal_write(sess, "hello, ");
    terminal_inverse(sess, true);
    terminal_write(sess, "world!");
    terminal_inverse(sess, false);

    // Parse input buffer
    int available;
    while ((available = terminal_available(sess)) > 0) {
        char val0 = terminal_read(sess), val1 = terminal_peek(sess, 0),
                val2 = terminal_peek(sess, 1);
        switch (available) {
        default:
        case 3:
            if (val0 == '\x1b' && val1 == '[' && val2 == 'A') {
                terminal_move(sess, 39, 0);
                terminal_write(sess, ".");
            } else if (val0 == '\x1b' && val1 == '[' && val2 == 'B') {
                terminal_move(sess, 39, 24);
                terminal_write(sess, ".");
            } else if (val0 == '\x1b' && val1 == '[' && val2 == 'C') {
                terminal_move(sess, 79, 12);
                terminal_write(sess, ".");
            } else if (val0 == '\x1b' && val1 == '[' && val2 == 'D') {
                terminal_move(sess, 0, 12);
                terminal_write(sess, ".");
            }
        }
    }

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
