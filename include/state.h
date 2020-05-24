#ifndef SSB_STATE_H
#define SSB_STATE_H

#include <time.h>
#include <stdbool.h>
#include "canvas.h"
#include "terminal.h"
#include "game.h"

enum screen {
    init_screen, title_screen, game_screen,
};

struct state {
    struct canvas canvas;
    struct terminal terminal;

    enum screen screen;

    long long tick_ms;
    struct timespec last_tick;
    size_t num_ticks;

    struct game game;
};

struct db;

bool state_create(struct state *state);

void state_destroy(struct state *state);

bool state_update(struct state *state, struct db *db);

#endif //SSB_STATE_H