#ifndef SSB_SCREEN_H
#define SSB_SCREEN_H

#include <stdbool.h>

struct env;
struct state;

struct screen_impl {
    void (*destroy)(void *screen, struct state *state);
    bool (*update)(void *screen, struct state *state, struct env *env);
};

struct screen {
    struct screen_impl *impl;

    unsigned char data[];
};

void screen_destroy(struct screen *screen, struct state *state);

bool screen_update(struct screen *screen, struct state *state, struct env *env);



extern struct screen_impl title_screen_impl;

struct screen *title_screen_create(struct state *state);

bool title_screen_update(void *data, struct state *state, struct env *env);


extern struct screen_impl game_screen_impl;

struct screen *game_screen_create(struct env *env, int level_id);

bool game_screen_update(void *data, struct state *state, struct env *env);


#endif //SSB_SCREEN_H
