#ifndef SSB_STATE_H
#define SSB_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <stdbool.h>
#include "canvas.h"
#include "terminal.h"
#include "game.h"
#include "env.h"

struct screen;

#define MAX_SCREENS 16

struct state {
    struct canvas canvas;
    struct terminal terminal;

    long long tick_ms;
    struct timespec last_tick;
    size_t num_ticks;

    int num_screens;
    struct screen *screens[MAX_SCREENS];
};

struct db;

bool state_create(struct state *state);

void state_destroy(struct state *state);

void state_set_tick_ms(struct state *state, long long tick_ms);

bool state_push_screen(struct state *state, struct screen *screen);

void state_clear_screens(struct state *state);

struct screen *state_peek_screen(struct state *state);

struct screen *state_pop_screen(struct state *state);

bool state_update(struct state *state, struct env *env);

#ifdef __cplusplus
}
#endif

#endif //SSB_STATE_H