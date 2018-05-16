#include <stdio.h>
#include "state.h"
#include "session.h"

void state_init(struct session *sess) {
    // Initialize the states
    sess->state.last_tick.tv_sec = sess->state.last_tick.tv_nsec = 0;
    sess->state.screen = game_screen;

    terminal_init(&sess->state.terminal_state);
    game_init(&sess->state.game_state);
    canvas_init(&sess->state.canvas, 80, 25);

    // Negotiate Telnet configuration
    terminal_send(sess, IAC WILL ECHO, 3);
    terminal_send(sess, IAC DONT ECHO, 3);
    terminal_send(sess, IAC WILL SUPPRESS_GO_AHEAD, 3);

    // Leave a greeting to describe the game version
    terminal_write(sess, WELCOME_MESSAGE);

    // Empty the terminal and hide the cursor
    terminal_clear(sess);
    terminal_move(sess, 0, 0);
    terminal_reset(sess);
    terminal_cursor(sess, false);
}

static bool title_screen_update(struct session *sess) {

    return true;
}

#define CANVAS &sess->state.canvas

static bool game_screen_update(struct session *sess) {
    static bool paused = false;

    terminal_read_menu_input(sess, &sess->state.game_state.input);

    canvas_rect(CANVAS, 0, 0, 80, 25, '#');

    static unsigned i = 1;
    unsigned x = i % 80, y = i / 80;
    canvas_put(CANVAS, x, y, '@');
    i += 1;

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
    bool disconnect = false;
    switch (sess->state.screen) {
    default:
    case title_screen:
        disconnect = title_screen_update(sess);
        break;

    case game_screen:
        disconnect = game_screen_update(sess);
        break;
    }

    // Render any updates to the screen
    //TODO find the max buffer that the connection can send
    char buf[512];
    size_t len;
    while (canvas_flush(CANVAS, buf, 512, &len)) {
        printf("%.*s\n", (int) len, buf);
        terminal_send(sess, buf, (int) len);
    }

    return disconnect;
}
