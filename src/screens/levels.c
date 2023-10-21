#include <stdlib.h>
#include <assert.h>
#include "../util.h"
#include "../db.h"
#include "../state.h"
#include "../screen.h"
#include "levels.h"
#include "game.h"
#include "log.h"
#include "replay.h"

struct screen_impl level_pit_screen_impl = {
        .update=level_pit_screen_update
};

struct level_pit_screen_state {
    uint32_t top_id;
    int selected_index;
};

struct screen *level_pit_screen_create(struct env *env) {
    struct screen *screen = malloc(sizeof(struct screen) + sizeof(struct level_pit_screen_state));
    *(struct level_pit_screen_state *)screen->data = (struct level_pit_screen_state){
            .top_id = 1,
            .selected_index = 0,
    };
    screen->impl = &level_pit_screen_impl;
    return screen;
}

static void draw_scrollbar(struct canvas *canvas, int x, int y, int w, int h, uint32_t first_id, uint32_t last_id, uint32_t max_id) {
    assert(w >= 3);
    assert(h >= 4);

    canvas_rect(canvas, x, y, w, h, '|');
    canvas_put(canvas, x + 1, y, '^');
    canvas_put(canvas, x + 1, y + 1, '=');
    canvas_put(canvas, x + 1, y + h - 2, '=');
    canvas_put(canvas, x + 1, y + h - 1, 'v');

    double const scrollbar_percentage_top = (double) first_id / max_id;
    double const scrollbar_percentage_bottom = (double) last_id / max_id;
    int const scrollbar_dragger_space = h - 4;
    int const scrollbar_dragger_y = (int)(scrollbar_percentage_top * scrollbar_dragger_space);
    int const scrollbar_dragger_height = SSB_CLAMP(
            (int)(scrollbar_percentage_bottom * scrollbar_dragger_space) - scrollbar_dragger_y,
            1,
            scrollbar_dragger_space - scrollbar_dragger_y);

    canvas_rect(canvas, x + 1, y + 2 + scrollbar_dragger_y, w - 2, scrollbar_dragger_height, '#');

}

size_t snprintf_time_in_ticks(char *buf, size_t len, uint32_t num_ticks) {
    uint32_t milliseconds = num_ticks * 100;
    uint32_t seconds = milliseconds / 1000;
    milliseconds %= 1000;
    uint32_t minutes = seconds / 60;
    seconds %= 60;
    return snprintf(buf, len, "%u:%02u.%01u", minutes, seconds, milliseconds / 100);
}

bool level_pit_screen_update(void *data, struct state *state, struct env *env) {
    struct level_pit_screen_state *screen = data;

    canvas_foreground(&state->canvas, white);
    canvas_background(&state->canvas, black);
    canvas_erase(&state->canvas);

    char *logo =
            " __      ______  __   ________  __           ______  __  ______ "
            "/\\ \\    /\\  ___\\/\\ \\ / /\\  ___\\/\\ \\         /\\  == \\/\\ \\/\\__  _\\"
            "\\ \\ \\___\\ \\  __\\\\ \\ \\'/\\ \\  __\\\\ \\ \\____    \\ \\  _-/\\ \\ \\/_/\\ \\/"
            " \\ \\_____\\ \\_____\\ \\__| \\ \\_____\\ \\_____\\    \\ \\_\\   \\ \\_\\ \\ \\_\\"
            "  \\/_____/\\/_____/\\/_/   \\/_____/\\/_____/     \\/_/    \\/_/  \\/_/";

    canvas_write_block(&state->canvas, 8, 0, 64, 5, logo);

    char *header =
            "    # NAME         CREATED    PLAYS   WINRATE BEST TIME"// AVG TIME "
            "===== ============ ========== ======= ======= =========";// =========";

    canvas_write_block(&state->canvas, 12, 6, 55, 2, header);

    struct metadata metadata[16];
    int const num_levels = db_get_metadata(env->db, screen->top_id - 1, metadata, 16);

    uint32_t min_id = 0;
    uint32_t max_id = 0;
    if (!db_get_level_bounds(env->db, &min_id, &max_id)) {
        LOG_ERROR("Failed to get level bounds");
        return false;
    }

    // Draw level list
    uint32_t selected_id = 0;
    for (int i = 0; i < num_levels; i++) {
        // Format the timestamps
        time_t const creation_timestamp = (time_t) metadata[i].creation_time;
        char creation_time_buf[32];
        strftime(creation_time_buf, sizeof(creation_time_buf), "%Y-%m-%d", gmtime(&creation_timestamp));

        char best_time_buf[32];
        if (metadata[i].num_wins == 0) {
            memcpy(best_time_buf, "-", 2);
        } else {
            snprintf_time_in_ticks(best_time_buf, sizeof(best_time_buf), metadata[i].min_ticks);
        }

        double win_rate = 0;
        if (metadata[i].num_attempts > 0) {
            win_rate = ((double)metadata[i].num_wins / metadata[i].num_attempts) * 100;
        }

        char buf[80];
        snprintf(buf, 80, "%5d %-12s %10s %7d  %5.01f%% %9s",
                metadata[i].id,
                metadata[i].name,
                creation_time_buf,
                metadata[i].num_attempts,
                win_rate,
                best_time_buf);

        if (i == screen->selected_index) {
            selected_id = metadata[i].id;

            canvas_foreground(&state->canvas, black);
            canvas_background(&state->canvas, white);
        }
        else {
            canvas_foreground(&state->canvas, white);
            canvas_background(&state->canvas, black);
        }

        canvas_write(&state->canvas, 12, 8 + i, buf);
    }

    canvas_foreground(&state->canvas, white);
    canvas_background(&state->canvas, black);

    // Draw scrollbar
    draw_scrollbar(&state->canvas, 72, 8, 3, 16,
                   (num_levels > 0 ? metadata[0].id : 0),
                   (num_levels > 0 ? metadata[num_levels-1].id : 0),
                   max_id);

    // Handle input
    if (state->terminal.keyboard.space || state->terminal.keyboard.enter) {
        state_push_screen(state, game_screen_create(env, selected_id));
    } else if (state->terminal.keyboard.up) {
        if (screen->selected_index > 0) {
            screen->selected_index--;
        } else if (screen->top_id > min_id) {
            screen->top_id = db_get_previous_level(env->db, screen->top_id);
        }
    } else if (state->terminal.keyboard.down) {
        if (screen->selected_index < num_levels - 1) {
            screen->selected_index++;
        } else if (num_levels == 16 && metadata[num_levels - 1].id < max_id) {
            screen->top_id = metadata[1].id;
        }
    } else if (state->terminal.keyboard.esc) {
        return false;
    } else if (KEYBOARD_KEY_PRESSED(state->terminal.keyboard, 'R')) {
        if (metadata[screen->selected_index].num_wins == 0) {
            LOG_ERROR("No wins");
        } else {
            uint32_t attempt_id = 0;
            if (!db_get_best_attempt(env->db, selected_id, &attempt_id)) {
                LOG_ERROR("Couldn't find best attempt for level %d", selected_id);
            } else {
                state_push_screen(state, replay_screen_create(env, attempt_id));
            }
        }
    }

    KEYBOARD_CLEAR(state->terminal.keyboard);

    return true;
}
