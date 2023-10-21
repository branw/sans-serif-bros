#include <stdlib.h>
#include <ctype.h>
#include "../state.h"
#include "../db.h"
#include "../screen.h"
#include "replay.h"
#include "log.h"

struct screen_impl replay_screen_impl = {
        .update = replay_screen_update
};

struct replay_screen_state {
    uint32_t attempt_id;
    struct attempt attempt;

    struct game game;

    char *next_input;
    uint32_t remaining_idles;
};

struct screen *replay_screen_create(struct env *env, uint32_t attempt_id) {
    struct screen *screen_base = malloc(sizeof(struct screen) + sizeof(struct replay_screen_state));

    struct replay_screen_state *screen = (struct replay_screen_state *)(screen_base->data);
    *screen = (struct replay_screen_state){
            .attempt_id = attempt_id,
    };
    screen_base->impl = &replay_screen_impl;

    if (!db_get_attempt(env->db, attempt_id, &screen->attempt)) {
        LOG_ERROR("Failed to find attempt %d", attempt_id);
        return false;
    }

    LOG_INFO("Replaying attempt %d of level %d with %d ticks:\n%s",
             attempt_id, screen->attempt.level_id, screen->attempt.ticks,
             screen->attempt.input_log);

    screen->next_input = screen->attempt.input_log;
    screen->remaining_idles = 0;

    char *field = NULL;
    if (!db_get_level_field_utf8(env->db, screen->attempt.level_id, &field)) {
        LOG_ERROR("Failed to get level");
        return false;
    }

    if (!game_create_from_utf8(&screen->game, field)) {
        LOG_ERROR("Failed to create game");
        return false;
    }

    free(field);

    return screen_base;
}

bool replay_screen_update(void *data, struct state *state, struct env *env) {
    struct replay_screen_state *screen = data;

    canvas_reset(&state->canvas);
    canvas_erase(&state->canvas);

    // Handle inputs
    if (KEYBOARD_KEY_PRESSED(state->terminal.keyboard, 'Q')) {
        return false;
    }

    KEYBOARD_CLEAR(state->terminal.keyboard);

    struct directional_input input = {0};
    char const next_input = screen->next_input[0];
    if (screen->remaining_idles > 0) {
        screen->remaining_idles--;
    } else if (isdigit(next_input)) {
        char *end = NULL;
        screen->remaining_idles = strtol(screen->next_input, &end, 10) - 1;
        screen->next_input = end;
    } else if (next_input != '\0') {
        switch (next_input) {
            case 'L':
                input.left = true;
                break;

            case 'R':
                input.right = true;
                break;

            case 'U':
                input.up = true;
                break;

            case 'D':
                input.down = true;
                break;

            default:
                LOG_ERROR("Invalid character in input log for attempt %d: %c",
                          screen->attempt_id, next_input);
                break;
        }

        screen->next_input++;
        if (screen->next_input[0] == '\0') {
            // Add one to the current ticks because we haven't actually
            // processed the last tick yet
            LOG_INFO("Done in %d ticks (attempt had %d ticks)",
                     screen->game.tick + 1, screen->attempt.ticks);
        }
    }

    enum game_state game_state = game_update(&screen->game, &input);

    canvas_foreground(&state->canvas, default_color);
    canvas_background(&state->canvas, default_color);

    // Draw the game field in the center of the canvas
    unsigned int const x_offset = (state->canvas.w - 80) / 2;
    unsigned int const y_offset = (state->canvas.h - 25) / 2;
    canvas_write_block_utf32(&state->canvas, x_offset, y_offset, 80, 25,
                             (uint32_t *) screen->game.field, ROWS * COLUMNS);

    // Color individual cells
    if (game_state == GAME_STATE_IN_PROGRESS) {
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
