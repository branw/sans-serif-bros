#include <stdlib.h>
#include <stdio.h>
#include "../state.h"
#include "../db.h"
#include "../screen.h"
#include "../server.h"
#include "../telnet.h"
#include "title.h"
#include "game.h"
#include "levels.h"
#include "log.h"

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
    terminal_write(&state->terminal, IAC DO NAWS);

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
                state_push_screen(state, game_screen_create(env, 166));
                return true;

            // level pit
            case 1:
                state_push_screen(state, level_pit_screen_create(env));
                return true;

            // instructions
            case 2:
                //TODO
                break;

            // quit game
            case 3:
                // Clean up the terminal before we kill the connection
                terminal_reset(&state->terminal);
                terminal_cursor(&state->terminal, true);

                return false;
        }
    }
    else if (state->terminal.keyboard.up && screen->selection > 0) {
        screen->selection--;
    }
    else if (state->terminal.keyboard.down && screen->selection < 3) {
        screen->selection++;
    }

    KEYBOARD_CLEAR(state->terminal.keyboard);

    canvas_reset(&state->canvas);
    canvas_erase(&state->canvas);

    unsigned int const x_offset = (state->canvas.w - 80) / 2;
    unsigned int const y_offset = (state->canvas.h - 25) / 2;

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

    // Pan the logo around
    unsigned tick = (state->num_ticks % 32) / 8;
    int dx = tick == 1 || tick == 2 ? 1 : 0;
    int dy = tick == 2 || tick == 3 ? 1 : 0;
    canvas_write_block(&state->canvas, x_offset + 7 + dx, y_offset + 1 + dy, 65, 13, logo);

    char buf[32];
    snprintf(buf, 32, "%d levels", db_num_levels(env->db));
    canvas_write(&state->canvas, x_offset + 4, y_offset + 12, buf);
    if (env->server) {
        if (env->server->num_sessions == 1) {
            canvas_write(&state->canvas, x_offset + 4, y_offset + 13, "1 player online");
        }
        else {
            snprintf(buf, 32, "%ld players online", env->server->num_sessions);
            canvas_write(&state->canvas, x_offset + 4, y_offset + 13, buf);
        }
    }
    else {
        canvas_write(&state->canvas, x_offset + 4, y_offset + 13, "single-player mode");
    }
    canvas_write(&state->canvas, x_offset + 4, y_offset + 14, "last updated 2023-09-30");

    canvas_write(&state->canvas, x_offset + 33, y_offset + 17, "classic mode");
    canvas_write(&state->canvas, x_offset + 33, y_offset + 19, "level pit");
    canvas_write(&state->canvas, x_offset + 33, y_offset + 21, "instructions");
    canvas_write(&state->canvas, x_offset + 33, y_offset + 23, "quit game");

    canvas_put(&state->canvas, x_offset + 30, y_offset + 17 + 2 * screen->selection, '>');

    return true;
}
