#include <time.h>
#include <stdio.h>
#include "state.h"
#include "db.h"
#include "telnet.h"

/*
 * Specific state handlers
 */

static void init_screen_update(struct state *state) {
    canvas_rect(&state->canvas, 2, 2, 4, 4, '#');

    // Negotiate Telnet configuration
    terminal_write(&state->terminal, IAC WILL ECHO);
    terminal_write(&state->terminal, IAC DONT ECHO);
    terminal_write(&state->terminal, IAC WILL SUPPRESS_GO_AHEAD);
    terminal_write(&state->terminal, IAC WILL NAWS);

    // Empty the terminal and hide the cursor
    terminal_clear(&state->terminal);
    terminal_move(&state->terminal, 0, 0);
    terminal_reset(&state->terminal);
    terminal_cursor(&state->terminal, false);

    state->screen = title_screen;
}

static bool title_screen_update(struct state *state, struct db *db) {
    struct level *level;
    if (!db_get_level(db, 1, &level)) {
        printf("failed to get level!\n");
        return false;
    }

    game_create(&state->game, level->field);

    state->screen = game_screen;

    return true;
}

static bool game_screen_update(struct state *state) {
    game_update(&state->game, &state->terminal.input);

    state->terminal.input.x = state->terminal.input.y = 0;

    if (state->game.win) {
        canvas_foreground(&state->canvas, white);
        canvas_background(&state->canvas, green);
    }

    canvas_write_block_utf32(&state->canvas, 0, 0, 80, 25,
                             (uint32_t *) state->game.field, ROWS * COLUMNS);

    return true;
}

/*
 * State
 */

bool state_create(struct state *state) {
    canvas_create(&state->canvas, 80, 25);
    terminal_create(&state->terminal, &state->canvas);

    state->screen = init_screen;

    state->tick_ms = 100;
    state->last_tick.tv_sec = state->last_tick.tv_nsec = 0;
    state->num_ticks = 0;

    return true;
}

void state_destroy(struct state *state) {
    terminal_destroy(&state->terminal);
    canvas_destroy(&state->canvas);
}

// Convert timespec to number in milliseconds
#define TO_MS(t) ((t).tv_sec * 1000 + (t).tv_nsec / 1000000)

bool state_update(struct state *state, struct db *db) {
    struct timespec current;
    clock_gettime(CLOCK_MONOTONIC, &current);

    // Check if a tick has elapsed
    long long delta = TO_MS(current) - TO_MS(state->last_tick);
    if (delta < state->tick_ms) {
        return true;
    }

    state->last_tick = current;
    state->num_ticks++;

    // Dispatch to the appropriate screen handler
    switch (state->screen) {
        case init_screen: {
            init_screen_update(state);
            // fallthrough
        }

        case title_screen: {
            return title_screen_update(state, db);
        }

        case game_screen: {
            return game_screen_update(state);
        }

        default: {
            fprintf(stderr, "unknown screen (%d)\n", (int)state->screen);
            return false;
        }
    }
}