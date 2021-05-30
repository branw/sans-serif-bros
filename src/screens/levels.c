#include <stdlib.h>
#include "util.h"
#include "db.h"
#include "state.h"
#include "screen.h"

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

    struct metadata *metadata[16];
    int const num_levels = db_get_metadata(env->db, screen->top_id, metadata, 16 + 1);

    for (int i = 0; i < MIN(num_levels, 16); i++) {
        char buf[80];
        snprintf(buf, 80, "%5d %-12s %10s %7d   %3.1u%% %9s",
                metadata[i]->id, (char *) metadata[i]->name, "2020-05-12",
                metadata[i]->num_plays, metadata[i]->num_wins / metadata[i]->num_plays,
                "0:34.2");

        if (metadata[i]->id == screen->selected_id) {
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

    bool has_more_levels = num_levels == 16 + 1;

    // Handle input
    if (state->terminal.keyboard.space || state->terminal.keyboard.enter) {
        state_push_screen(state, game_screen_create(env, screen->selected_id));
    }
    else if (state->terminal.keyboard.up && screen->selected_id > 1) {
        screen->selected_id--;
    }
    else if (state->terminal.keyboard.down && screen->selected_id < 4000) {
        screen->selected_id++;
    }

    KEYBOARD_CLEAR(state->terminal.keyboard);

    return true;
}
