#ifndef SSB_SCREEN_GAME_H
#define SSB_SCREEN_GAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

struct env;
struct state;

struct screen *game_screen_create(struct env *env, uint32_t level_id);

bool game_screen_update(void *data, struct state *state, struct env *env);

#ifdef __cplusplus
}
#endif

#endif //SSB_SCREEN_GAME_H
