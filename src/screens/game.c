#include <stdlib.h>
#include "state.h"
#include "db.h"
#include "screen.h"


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
    } else if (KEYBOARD_KEY_PRESSED(state->terminal.keyboard, 'Q')) {
        return false;
    } else if (state->terminal.keyboard.space && screen->game.win) {
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

    // Draw the game field in the center of the canvas
    unsigned int const x_offset = (state->canvas.w - 80) / 2;
    unsigned int const y_offset = (state->canvas.h - 25) / 2;
    canvas_write_block_utf32(&state->canvas, x_offset, y_offset, 80, 25,
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
                canvas_put(&state->canvas, x_offset + x, y_offset + y, ch);
            }
        }
    }

    // Draw instructions
    if (screen->game.die && state->num_ticks % 20 < 10) {
        canvas_foreground(&state->canvas, red);
        canvas_background(&state->canvas, black);

        canvas_fill(&state->canvas, x_offset + 29, y_offset + 9, 22, 7, ' ');
        canvas_rect(&state->canvas, x_offset + 30, y_offset + 10, 20, 5, '#');
        canvas_write(&state->canvas, x_offset + 32, y_offset + 12, "press R to retry");
    }
    else if (screen->game.win && state->num_ticks % 20 < 10) {
        canvas_foreground(&state->canvas, green);
        canvas_background(&state->canvas, black);

        canvas_fill(&state->canvas, x_offset + 26, y_offset + 9, 29, 7, ' ');
        canvas_rect(&state->canvas, x_offset + 27, y_offset + 10, 27, 5, '#');
        canvas_write(&state->canvas, x_offset + 29, y_offset + 12, "press space to continue");
    }

    // Resend the entire canvas every so many ticks
    if (state->num_ticks % 100 == 99) {
        canvas_force_next_flush(&state->canvas);
    }

    return true;
}
