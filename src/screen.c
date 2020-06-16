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
            case 0:
                state_push_screen(state, game_screen_create(env, 1));
                return true;

            // level pit
            case 1:
                state_push_screen(state, level_pit_screen_create(env));
                return true;

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
    snprintf(buf, 32, "%d levels", db_num_levels(env->db));
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
    int level_id;
    struct game game;

    int transition_ticks;
};

struct screen *game_screen_create(struct env *env, int level_id) {
    struct screen *screen = malloc(sizeof(struct screen) + sizeof(struct game_screen_state));
    *(struct game_screen_state *)screen->data = (struct game_screen_state){
        .level_id = level_id,
        .game = {0},
        .transition_ticks = -1,
    };
    screen->impl = &game_screen_impl;

    // Load the level from the database
    struct level *level;
    if (!db_get_level(env->db, level_id, &level)) {
        printf("failed to get level!\n");
        return false;
    }

    game_create(&((struct game_screen_state *)screen->data)->game, level->field);

    return screen;
}

bool game_screen_update(void *data, struct state *state, struct env *env) {
    struct game_screen_state *screen = data;

    // Allow flashes of color during transitions
    bool color = true;

    // Handle inputs
    if (KEYBOARD_KEY_PRESSED(state->terminal.keyboard, 'R')) {
        struct level *level;
        if (!db_get_level(env->db, screen->level_id, &level)) {
            printf("failed to get level!\n");
            return false;
        }

        game_create(&screen->game, level->field);

        color = false;
        canvas_foreground(&state->canvas, blue);
        canvas_background(&state->canvas, blue);
    }
    else if (state->terminal.keyboard.space && screen->game.win) {
        struct level *level;
        if (!db_get_level(env->db, ++screen->level_id, &level)) {
            printf("failed to get level!\n");
            return false;
        }

        game_create(&screen->game, level->field);

        color = false;
        canvas_foreground(&state->canvas, green);
        canvas_background(&state->canvas, green);
    }

    struct directional_input input;
    terminal_get_directional_input(&state->terminal, &input, false);

    game_update(&screen->game, &input);

    KEYBOARD_CLEAR(state->terminal.keyboard);

    // Color based on the current state
    if (color && screen->game.win) {
        canvas_foreground(&state->canvas, black);
        canvas_background(&state->canvas, green);
    }
    else if (color && screen->game.die) {
        canvas_foreground(&state->canvas, black);
        canvas_background(&state->canvas, red);
    }
    else if (color) {
        canvas_foreground(&state->canvas, white);
        canvas_background(&state->canvas, black);
    }

    // Draw the game field
    canvas_write_block_utf32(&state->canvas, 0, 0, 80, 25,
                             (uint32_t *) screen->game.field, ROWS * COLUMNS);

    // Color individual cells
    if (color && !screen->game.win && !screen->game.die) {
        for (unsigned y = 0; y < 25; y++) {
            for (unsigned x = 0; x < 80; x++) {
                unsigned long ch = screen->game.field[y][x];
                switch (ch) {
                    case 'I':
                        canvas_foreground(&state->canvas, black);
                        canvas_background(&state->canvas, blue);
                        break;

                    case 0xa3:
                    case 'E':
                        canvas_foreground(&state->canvas, black);
                        canvas_background(&state->canvas, green);
                        break;

                    case '[':
                    case ']':
                    case '{':
                    case '}':
                    case 'X':
                    case '%':
                        canvas_foreground(&state->canvas, black);
                        canvas_background(&state->canvas, red);
                        break;

                    default: continue;
                }
                canvas_put(&state->canvas, x, y, ch);
            }
        }
    }

    // Draw instructions
    if (screen->game.die && state->num_ticks % 20 < 10) {
        canvas_foreground(&state->canvas, red);
        canvas_background(&state->canvas, black);

        canvas_fill(&state->canvas, 29, 9, 22, 7, ' ');
        canvas_rect(&state->canvas, 30, 10, 20, 5, '#');
        canvas_write(&state->canvas, 32, 12, "press R to retry");
    }
    else if (screen->game.win && state->num_ticks % 20 < 10) {
        canvas_foreground(&state->canvas, green);
        canvas_background(&state->canvas, black);

        canvas_fill(&state->canvas, 26, 9, 29, 7, ' ');
        canvas_rect(&state->canvas, 27, 10, 27, 5, '#');
        canvas_write(&state->canvas, 29, 12, "press space to continue");
    }

    // Resend the entire canvas every so many ticks
    if (state->num_ticks % 100 == 99) {
        canvas_force_next_flush(&state->canvas);
    }

    return true;
}


struct screen_impl level_pit_screen_impl = {
        .update=level_pit_screen_update
};

struct level_pit_screen_state {
    int top_id;
    int selected_id;
};

struct screen *level_pit_screen_create(struct env *env) {
    struct screen *screen = malloc(sizeof(struct screen) + sizeof(struct level_pit_screen_state));
    *(struct level_pit_screen_state *)screen->data = (struct level_pit_screen_state){
        .top_id = 1,
        .selected_id = 1,
    };
    screen->impl = &level_pit_screen_impl;
    return screen;
}

bool level_pit_screen_update(void *data, struct state *state, struct env *env) {
    struct level_pit_screen_state *screen = data;

    // Handle input
    if (state->terminal.keyboard.space || state->terminal.keyboard.enter) {
        state_push_screen(state, game_screen_create(env, screen->selected_id));
    }
    else if (state->terminal.keyboard.up && screen->selected_id > 0) {
        screen->selected_id--;
    }
    else if (state->terminal.keyboard.down && screen->selected_id < 4000) {
        screen->selected_id++;
    }

    KEYBOARD_CLEAR(state->terminal.keyboard);

    canvas_erase(&state->canvas);

    char *logo =
            " __      ______  __   ________  __           ______  __  ______ "
            "/\\ \\    /\\  ___\\/\\ \\ / /\\  ___\\/\\ \\         /\\  == \\/\\ \\/\\__  _\\"
            "\\ \\ \\___\\ \\  __\\\\ \\ \\'/\\ \\  __\\\\ \\ \\____    \\ \\  _-/\\ \\ \\/_/\\ \\/"
            " \\ \\_____\\ \\_____\\ \\__| \\ \\_____\\ \\_____\\    \\ \\_\\   \\ \\_\\ \\ \\_\\"
            "  \\/_____/\\/_____/\\/_/   \\/_____/\\/_____/     \\/_/    \\/_/  \\/_/";

    canvas_write_block(&state->canvas, 8, 0, 64, 5, logo);

    char *header =
            "    # NAME         CREATED    PLAYS   WINRATE BEST TIME"
            "===== ============ ========== ======= ======= =========";

    canvas_write_block(&state->canvas, 12, 6, 55, 2, header);

    /*
    int selected_id_index = 0;
    void *node = NULL;
    struct metadata metadata = {.id = screen->top_id};
    for (int i = 0; i < 16; i++) {
        if (!db_next_level(env->db, &node, &metadata)) {
            break;
        }

        char buf[80];
        snprintf(buf, 80, "%5d %-12s %10s %7d   %3.1f%% %9s",
                metadata.id, "foobar", "2020-05-12", 999999, 99.9f, "0:34.2");

        if (metadata.id == screen->selected_id) {
            canvas_foreground(&state->canvas, black);
            canvas_background(&state->canvas, white);
        }
        else {
            canvas_foreground(&state->canvas, white);
            canvas_background(&state->canvas, black);
        }

        canvas_write(&state->canvas, 12, 8 + i, buf);
    }
     */

    canvas_foreground(&state->canvas, white);
    canvas_background(&state->canvas, black);

    return true;
}
