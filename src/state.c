#include <time.h>
#include <stdio.h>
#include "state.h"
#include "db.h"
#include "telnet.h"
#include "env.h"
#include "server.h"

/*
 * Specific state handlers
 */

static void init_screen_update(struct state *state) {
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

static bool title_screen_update(struct state *state, struct env *env) {
    // Handle input
    if (state->terminal.keyboard.space || state->terminal.keyboard.enter) {
        printf("%d %d\n", state->terminal.keyboard.space, state->terminal.keyboard.enter);

        switch (state->selection) {
            // classic mode
            case 0: {
                struct level *level;
                if (!db_get_level(env->db, 1, &level)) {
                    printf("failed to get level!\n");
                    return false;
                }

                game_create(&state->game, level->field);

                state->screen = game_screen;
                return true;
            }

            // level pit
            case 1:

                break;

            // instructions
            case 2:

                break;

        }
    }
    else if (state->terminal.keyboard.up && state->selection > 0) {
        state->selection--;
    }
    else if (state->terminal.keyboard.down && state->selection < 2) {
        state->selection++;
    }

    KEYBOARD_CLEAR(state->terminal.keyboard);

    canvas_erase(&state->canvas);

    // Draw logo
    char *logo =
            " ______    ______    __   __    ______                           "
            "/\\  ___\\  /\\  __ \\  /\\ \"-.\\ \\  /\\  ___\\                          "
            "\\ \\___  \\ \\ \\  __ \\ \\ \\ \\-.  \\ \\ \\___  \\                         "
            " \\/\\_____\\ \\ \\_\\ \\_\\ \\ \\_\\\\\"\\_\\ \\/\\_____\\                        "
            "  \\/_____/  \\/_/\\/_/  \\/_/_\\/_/  \\/_____/   __    ______         "
            "             /\\  ___\\  /\\  ___\\  /\\  == \\  /\\ \\  /\\  ___\\        "
            "             \\ \\___  \\ \\ \\  __\\  \\ \\  __<  \\ \\ \\ \\ \\  __\\        "
            "              \\/\\_____\\ \\ \\_____\\ \\ \\_\\ \\_\\ \\ \\_\\ \\ \\_\\          "
            "               \\/_____/  \\/_____/  \\/_/_/_/  \\/_/__\\/_/ ______   "
            "                         /\\  == \\  /\\  == \\  /\\  __ \\  /\\  ___\\  "
            "                         \\ \\  __<  \\ \\  __<  \\ \\ \\/\\ \\ \\ \\___  \\ "
            "                          \\ \\_____\\ \\ \\_\\ \\_\\ \\ \\_____\\ \\/\\_____\\"
            "                           \\/_____/  \\/_/ /_/  \\/_____/  \\/_____/";

    unsigned tick = (state->num_ticks % 16) / 4;
    int dx = tick == 1 || tick == 2 ? 1 : 0;
    int dy = tick == 2 || tick == 3 ? 1 : 0;
    canvas_write_block(&state->canvas, 7 + dx, 1 + dy, 65, 13, logo);

    char buf[32];
    snprintf(buf, 32, "%d levels", env->db->num_levels);
    canvas_write(&state->canvas, 4, 12, buf);
    if (env->server) {
        if (env->server->num_sessions == 1) {
            canvas_write(&state->canvas, 4, 13, "1 player online");
        }
        else {
            snprintf(buf, 32, "%ld players online", env->server->num_sessions);
            canvas_write(&state->canvas, 4, 13, buf);
        }
    }
    else {
        canvas_write(&state->canvas, 4, 13, "single-player mode");
    }
    canvas_write(&state->canvas, 4, 14, "last updated 2020-5-29");

    canvas_write(&state->canvas, 33, 17, "classic mode");
    canvas_write(&state->canvas, 33, 19, "level pit");
    canvas_write(&state->canvas, 33, 21, "instructions");

    canvas_put(&state->canvas, 30, 17 + 2 * state->selection, '>');

    return true;
}

static bool game_screen_update(struct state *state) {
    struct directional_input input;
    terminal_get_directional_input(&state->terminal, &input, false);

    game_update(&state->game, &input);

    KEYBOARD_CLEAR(state->terminal.keyboard);

    if (state->game.win) {
        canvas_foreground(&state->canvas, black);
        canvas_background(&state->canvas, green);
    }
    else {
        canvas_foreground(&state->canvas, white);
        canvas_background(&state->canvas, black);
    }

    canvas_write_block_utf32(&state->canvas, 0, 0, 80, 25,
                             (uint32_t *) state->game.field, ROWS * COLUMNS);

    // Resend the entire canvas every so many ticks
    if (state->num_ticks % 100 == 99) {
        canvas_force_next_flush(&state->canvas);
    }

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

    state->selection = 0;

    return true;
}

void state_destroy(struct state *state) {
    terminal_destroy(&state->terminal);
    canvas_destroy(&state->canvas);
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

    // Dispatch to the appropriate screen handler
    switch (state->screen) {
        case init_screen: {
            init_screen_update(state);
            // fallthrough
        }

        case title_screen: {
            return title_screen_update(state, env);
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