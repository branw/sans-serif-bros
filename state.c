#include <stdio.h>
#include "state.h"
#include "session.h"
#include "util.h"

void state_init(struct session *sess) {
    // Initialize the states
    sess->state.last_tick.tv_sec = sess->state.last_tick.tv_nsec = 0;
    sess->state.screen = game_screen;
    terminal_init(&sess->state.terminal_state);
    game_init(&sess->state.game_state);

    // Negotiate Telnet configuration
    terminal_send(sess, IAC WILL ECHO, 3);
    terminal_send(sess, IAC DONT ECHO, 3);
    terminal_send(sess, IAC WILL SUPPRESS_GO_AHEAD, 3);

    // Leave a greeting to describe the game version
    terminal_write(sess, WELCOME_MESSAGE);

    // Empty the terminal and hide the cursor
    terminal_clear(sess);
    terminal_reset(sess);
    terminal_cursor(sess, false);
}

static bool title_screen_update(struct session *sess) {
    terminal_move(sess, 0, 0);
    terminal_rect(sess, 0, 0, 80, 25, '#');

    // Parse input buffer
    struct menu_input input;
    terminal_read_menu_input(sess, &input);

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

    //sess->state.screen = game_screen;

    return true;
}

static bool game_screen_update(struct session *sess) {
    static bool paused = false;

    terminal_read_menu_input(sess, &sess->state.game_state.input);

    if (sess->state.game_state.input.enter) {
        game_init(&sess->state.game_state);
    }

    if (!paused) {
        game_update(&sess->state.game_state);
    }

    terminal_reset(sess);

    if (sess->state.game_state.win) {
        terminal_write(sess, "\x1b[42m");
    } else if (sess->state.game_state.die) {
        terminal_write(sess, "\x1b[41m");
    }

    if (sess->state.game_state.input.esc) {
        paused = !paused;
    }

    if (paused) {
        terminal_write(sess, "\x1b[30;1m");
    }

    unsigned off_x = 2, off_y = 2;

    for (unsigned row = 0; row < ROWS; ++row) {
        terminal_move(sess, off_x + 0, off_y + row);
        if (row == ROWS - 1) {
            terminal_underline(sess, true);
        }

        // This works when the client is using CP-1252:
        //terminal_send(sess, (char *)(sess->state.game_state.field[row]),
        //              sizeof(unsigned long) * COLUMNS);

        // This works when the client is using UTF-8:
        char line[4 * COLUMNS] = {0}, *end = &line[4 * COLUMNS - 1], *p = line;

        for (unsigned column = 0; column < COLUMNS; ++column) {
            utf8_encode(&p, end, sess->state.game_state.field[row][column]);
        }

        terminal_write(sess, line);
    }

    terminal_reset(sess);

    terminal_move(sess, off_x, off_y + ROWS);
    terminal_write(sess, "[Level 1]");

    terminal_pretty_rect(sess, off_x - 1, off_y - 1, COLUMNS + 2, ROWS + 3);

    if (paused) {
        terminal_rect(sess, 20, 5, 30, 12, '#');
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

    case game_screen:
        return game_screen_update(sess);
    }
}
