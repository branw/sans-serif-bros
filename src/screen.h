#ifndef SSB_SCREEN_H
#define SSB_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif //SSB_SCREEN_H
