#include <time.h>
#include <stdio.h>
#include "state.h"
#include "db.h"
#include "env.h"
#include "server.h"
#include "screen.h"

/*
 * Specific state handlers
 */

/*
static bool game_screen_update(struct state *state) {

}
 */



/*
 * State
 */

bool state_create(struct state *state) {
    canvas_create(&state->canvas, 80, 25);
    terminal_create(&state->terminal, &state->canvas);

    state->tick_ms = 100;
    state->last_tick.tv_sec = state->last_tick.tv_nsec = 0;
    state->num_ticks = 0;

    state_clear_screens(state);
    state_push_screen(state, title_screen_create(state));

    return true;
}

void state_destroy(struct state *state) {
    terminal_destroy(&state->terminal);
    canvas_destroy(&state->canvas);
}

void state_set_tick_ms(struct state *state, long long tick_ms) {
    state->tick_ms = tick_ms;
}

bool state_push_screen(struct state *state, struct screen *screen) {
    if (state->num_screens == MAX_SCREENS) {
        return false;
    }

    KEYBOARD_CLEAR(state->terminal.keyboard);

    state->screens[state->num_screens++] = screen;
    return true;
}

void state_clear_screens(struct state *state) {
    state->num_screens = 0;
}

struct screen *state_peek_screen(struct state *state) {
    return state->screens[state->num_screens - 1];
}

struct screen *state_pop_screen(struct state *state) {
    if (state->num_screens == 0) {
        return NULL;
    }

    return state->screens[--state->num_screens];
}

// Convert timespec to number in milliseconds
#define TO_MS(t) ((t).tv_sec * 1000 + (t).tv_nsec / 1000000)

bool state_update(struct state *state, struct env *env) {
    struct timespec current;
    clock_gettime(CLOCK_MONOTONIC, &current);

    // Check if a tick has elapsed
    long long delta = TO_MS(current) - TO_MS(state->last_tick);
    if (delta < state->tick_ms) {
        return true;
    }

    state->last_tick = current;
    state->num_ticks++;

    struct screen *screen;
    while ((screen = state_peek_screen(state)) &&
            screen != NULL && !screen_update(screen, state, env)) {
        screen_destroy(state_pop_screen(state), state);
    }

    return (screen != NULL);
}
