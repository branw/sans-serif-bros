#ifndef SSB_SCREEN_TITLE_H
#define SSB_SCREEN_TITLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

struct env;
struct state;

struct screen *title_screen_create(struct state *state);

bool title_screen_update(void *data, struct state *state, struct env *env);

#ifdef __cplusplus
}
#endif

#endif //SSB_SCREEN_TITLE_H
