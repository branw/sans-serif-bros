#ifndef SSB_SCREEN_LEVELS_H
#define SSB_SCREEN_LEVELS_H

#include <stdbool.h>

struct env;
struct state;

struct screen *level_pit_screen_create(struct env *env);

bool level_pit_screen_update(void *data, struct state *state, struct env *env);

#endif //SSB_SCREEN_LEVELS_H
