#include <stdlib.h>
#include "../state.h"
#include "../db.h"
#include "../screen.h"
#include "game.h"
#include "log.h"

struct screen_impl game_screen_impl = {
        .update = game_screen_update
};

struct game_screen_state {
    uint32_t level_id;
    struct game game;

    enum game_state last_game_state;

    int transition_ticks;
};

struct screen *game_screen_create(struct env *env, uint32_t level_id) {
    struct screen *screen = malloc(sizeof(struct screen) + sizeof(struct game_screen_state));
    *(struct game_screen_state *)(screen->data) = (struct game_screen_state){
            .level_id = level_id,
            .game = {0},
            .last_game_state = GAME_STATE_IN_PROGRESS,
            .transition_ticks = -1,
    };
    screen->impl = &game_screen_impl;

    // Load the level from the database
    char *field = NULL;
    if (!db_get_level_field_utf8(env->db, level_id, &field)) {
        LOG_ERROR("Failed to get level");
        return false;
    }

    if (!game_create_from_utf8(&((struct game_screen_state *)screen->data)->game, field)) {
        LOG_ERROR("Failed to create game");
        return false;
    }

    free(field);

    return screen;
}

bool game_screen_update(void *data, struct state *state, struct env *env) {
    struct game_screen_state *screen = data;

    canvas_reset(&state->canvas);
    canvas_erase(&state->canvas);

    // Allow flashes of color during transitions
    bool color = true;

    // If the player is actively playing, consider this a legitimate attempt
    // and record it, even if the player retries or quits
    bool const should_record_attempt = screen->game.input_log_len > 5 &&
            !(screen->game.win || screen->game.die);

    // Handle inputs
    if (KEYBOARD_KEY_PRESSED(state->terminal.keyboard, 'R')) {
        struct attempt attempt = {
                .game_state = GAME_STATE_RETRIED,
                .level_id = screen->level_id,
                .ticks = screen->game.tick,
                .input_log = screen->game.input_log,
        };
        if (should_record_attempt && !db_insert_attempt(env->db, &attempt)) {
            LOG_ERROR("Failed to record retried attempt");
        }

        char *field = NULL;
        if (!db_get_level_field_utf8(env->db, screen->level_id, &field)) {
            LOG_ERROR("Failed to get level %d for retry", screen->level_id);
            return false;
        }

        if (!game_create_from_utf8(&screen->game, field)) {
            LOG_ERROR("Failed to create game for retry of level %d", screen->level_id);
            return false;
        }

        free(field);

        color = false;
        canvas_foreground(&state->canvas, blue);
        canvas_background(&state->canvas, blue);
    } else if (KEYBOARD_KEY_PRESSED(state->terminal.keyboard, 'Q')) {
        struct attempt attempt = {
                .game_state = GAME_STATE_QUIT,
                .level_id = screen->level_id,
                .ticks = screen->game.tick,
                .input_log = screen->game.input_log,
        };
        if (should_record_attempt && !db_insert_attempt(env->db, &attempt)) {
            LOG_ERROR("Failed to record quit attempt");
        }

        return false;
    } else if (state->terminal.keyboard.space && screen->game.win) {
        char *field = NULL;
        if (!db_get_level_field_utf8(env->db, ++screen->level_id, &field)) {
            printf("failed to get level!");
            return false;
        }

        if (!game_create_from_utf8(&screen->game, field)) {
            LOG_ERROR("Failed to create game");
            return false;
        }

        free(field);

        color = false;
        canvas_foreground(&state->canvas, green);
        canvas_background(&state->canvas, green);
    }

    struct directional_input input;
    terminal_get_directional_input(&state->terminal, &input, false);

    enum game_state game_state = game_update(&screen->game, &input);
    if (game_state != screen->last_game_state) {
        if (game_state != GAME_STATE_IN_PROGRESS) {
            struct attempt attempt = {
                    .game_state = game_state,
                    .level_id = screen->level_id,
                    .ticks = screen->game.tick,
                    .input_log = screen->game.input_log,
            };
            if (!db_insert_attempt(env->db, &attempt)) {
                LOG_ERROR("Failed to record attempt");
            }
        }

        screen->last_game_state = game_state;
    }

    KEYBOARD_CLEAR(state->terminal.keyboard);

    // Color based on the current state
    if (color && game_state == GAME_STATE_WON) {
        canvas_foreground(&state->canvas, black);
        canvas_background(&state->canvas, green);
    }
    else if (color && game_state == GAME_STATE_DIED) {
        canvas_foreground(&state->canvas, black);
        canvas_background(&state->canvas, red);
    }
    else if (color) {
        canvas_foreground(&state->canvas, default_color);
        canvas_background(&state->canvas, default_color);
    }

    // Draw the game field in the center of the canvas
    unsigned int const x_offset = (state->canvas.w - 80) / 2;
    unsigned int const y_offset = (state->canvas.h - 25) / 2;
    canvas_write_block_utf32(&state->canvas, x_offset, y_offset, 80, 25,
                             (uint32_t *) screen->game.field, ROWS * COLUMNS);

    // Color individual cells
    if (color && game_state == GAME_STATE_IN_PROGRESS) {
        for (unsigned y = 0; y < 25; y++) {
            for (unsigned x = 0; x < 80; x++) {
                unsigned long ch = screen->game.field[y][x];
                switch (ch) {
                    case 'I':
                        canvas_foreground(&state->canvas, white);
                        canvas_background(&state->canvas, blue);
                        break;

                    case 0xa3:
                    case 'E':
                        canvas_foreground(&state->canvas, white);
                        canvas_background(&state->canvas, green);
                        break;

                    case '[':
                    case ']':
                    case '{':
                    case '}':
                    case 'X':
                    case '%':
                        canvas_foreground(&state->canvas, white);
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

    // Draw a border around the game
    if (x_offset > 2 && y_offset > 3) {
        canvas_foreground(&state->canvas, default_color);
        canvas_background(&state->canvas, default_color);

        //canvas_rect(&state->canvas, x_offset - 1, y_offset - 1, COLUMNS + 2, ROWS + 2, ' ');

        canvas_line(&state->canvas, x_offset, y_offset - 1, x_offset + COLUMNS - 1, y_offset - 1, '-');
        canvas_line(&state->canvas, x_offset, y_offset + ROWS, x_offset + COLUMNS - 1, y_offset + ROWS, '-');
        canvas_line(&state->canvas, x_offset - 1, y_offset, x_offset - 1, y_offset + ROWS - 1, '|');
        canvas_line(&state->canvas, x_offset + COLUMNS, y_offset, x_offset + COLUMNS, y_offset + ROWS - 1, '|');
        for (int horiz = 0; horiz < 2; horiz++) {
            for (int vert = 0; vert < 2; vert++) {
                canvas_put(&state->canvas,
                           x_offset - 1 + horiz * (COLUMNS + 1),
                           y_offset - 1 + vert * (ROWS + 1),
                           '+');
            }
        }

        char buf[128] = {0};
        snprintf(buf, sizeof(buf), "%5d ticks", screen->game.tick);

        canvas_write(&state->canvas, x_offset, y_offset + ROWS + 1, buf);

        screen->game.input_log[screen->game.input_log_len] = '\0';
        canvas_write_block(&state->canvas, x_offset, y_offset + ROWS + 2, COLUMNS, 3, screen->game.input_log);
    }

    // Resend the entire canvas every so many ticks
    if (state->num_ticks % 100 == 99) {
        canvas_force_next_flush(&state->canvas);
    }

    return true;
}
