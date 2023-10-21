#ifndef SSB_SCREEN_REPLAY_H
#define SSB_SCREEN_REPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

struct env;
struct state;

struct screen *replay_screen_create(struct env *env, uint32_t attempt_id);

bool replay_screen_update(void *data, struct state *state, struct env *env);

#ifdef __cplusplus
}
#endif

#endif //SSB_SCREEN_REPLAY_H
