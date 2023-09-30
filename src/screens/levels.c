#include <stdlib.h>
#include "../util.h"
#include "../db.h"
#include "../state.h"
#include "../screen.h"
#include "levels.h"
#include "game.h"
#include "log.h"

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

bool level_pit_screen_update(void *data, struct state *state, struct env *env) {
    struct level_pit_screen_state *screen = data;

    //TODO no need to update this every tick
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

    struct metadata metadata[16];
    int const num_levels = db_get_metadata(env->db, screen->top_id - 1, metadata, 16);

    uint32_t min_id = 0;
    uint32_t max_id = 0;
    if (!db_get_level_bounds(env->db, &min_id, &max_id)) {
        LOG_ERROR("Failed to get level bounds");
        return false;
    }

    uint32_t selected_id = 0;
    uint32_t const last_id = metadata[num_levels - 1].id;
    for (int i = 0; i < num_levels; i++) {
        char buf[80];
        snprintf(buf, 80, "%5d %-12s %10s %7d   %3.1u%% %9s",
                metadata[i].id,
                (char *) metadata[i].name,
                "2020-05-12",
                metadata[i].num_plays,
                metadata[i].num_plays == 0 ? 0 : metadata[i].num_wins / metadata[i].num_plays,
                "0:34.2");

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

    // Handle input
    if (state->terminal.keyboard.space || state->terminal.keyboard.enter) {
        state_push_screen(state, game_screen_create(env, selected_id));
    }
    else if (state->terminal.keyboard.up) {
        if (screen->selected_index > 0) {
            screen->selected_index--;
        } else if (screen->top_id > min_id) {
            screen->top_id = db_get_previous_level(env->db, screen->top_id);
        }
    }
    else if (state->terminal.keyboard.down) {
        if (screen->selected_index < 15) {
            screen->selected_index++;
        } else if (num_levels == 16) {
            screen->top_id = metadata[1].id;
        }
    } else if (state->terminal.keyboard.esc) {
        return false;
    }

    KEYBOARD_CLEAR(state->terminal.keyboard);

    return true;
}
