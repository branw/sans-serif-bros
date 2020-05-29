#include <stdlib.h>
#include <string.h>
#include "screen.h"
#include "canvas.h"
#include "terminal.h"
#include "telnet.h"
#include "state.h"
#include "server.h"
#include "db.h"

void screen_destroy(struct screen *screen, struct state *state) {
    if (screen->impl->destroy) {
        screen->impl->destroy(screen->data, state);
    }
    free(screen);
}

bool screen_update(struct screen *screen, struct state *state, struct env *env) {
    return screen->impl->update(screen->data, state, env);
}



struct screen_impl title_screen_impl = {
        .update=title_screen_update,
};

struct title_screen_state {
    int selection;
};

struct screen *title_screen_create(struct state *state) {
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

    struct screen *screen = malloc(sizeof(struct screen) + sizeof(struct title_screen_state));
    screen->impl = &title_screen_impl;
    *(struct title_screen_state *)screen->data = (struct title_screen_state){
            .selection = 0,
    };
    return screen;
}

bool title_screen_update(void *data, struct state *state, struct env *env) {
    struct title_screen_state *screen = data;

    // Handle input
    if (state->terminal.keyboard.space || state->terminal.keyboard.enter) {
        switch (screen->selection) {
            // classic mode
            case 0: {
                state_push_screen(state, game_screen_create(env, 1));
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
    else if (state->terminal.keyboard.up && screen->selection > 0) {
        screen->selection--;
    }
    else if (state->terminal.keyboard.down && screen->selection < 2) {
        screen->selection++;
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

    canvas_put(&state->canvas, 30, 17 + 2 * screen->selection, '>');

    return true;
}




struct screen_impl game_screen_impl = {
        .update = game_screen_update
};

struct game_screen_state {
    struct game game;
};

struct screen *game_screen_create(struct env *env, int level_id) {
    struct level *level;
    if (!db_get_level(env->db, level_id, &level)) {
        printf("failed to get level!\n");
        return false;
    }

    struct screen *screen = malloc(sizeof(struct screen) + sizeof(struct game_screen_state));
    *(struct game_screen_state *)screen->data = (struct game_screen_state){0};
    screen->impl = &game_screen_impl;

    game_create(&((struct game_screen_state *)screen->data)->game, level->field);

    return screen;
}

bool game_screen_update(void *data, struct state *state, struct env *env) {
    struct game_screen_state *screen = data;

    struct directional_input input;
    terminal_get_directional_input(&state->terminal, &input, false);

    game_update(&screen->game, &input);

    KEYBOARD_CLEAR(state->terminal.keyboard);

    if (screen->game.win) {
        canvas_foreground(&state->canvas, black);
        canvas_background(&state->canvas, green);
    }
    else {
        canvas_foreground(&state->canvas, white);
        canvas_background(&state->canvas, black);
    }

    canvas_write_block_utf32(&state->canvas, 0, 0, 80, 25,
                             (uint32_t *) screen->game.field, ROWS * COLUMNS);

    // Resend the entire canvas every so many ticks
    if (state->num_ticks % 100 == 99) {
        canvas_force_next_flush(&state->canvas);
    }

    return true;
}
