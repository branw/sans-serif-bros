#include <stdlib.h>
#include <string.h>
#include "screen.h"
#include "canvas.h"
#include "terminal.h"
#include "telnet.h"
#include "state.h"
#include "server.h"
#include "db.h"

void screen_destroy(struct screen *screen, struct state *state) {
    if (screen->impl->destroy) {
        screen->impl->destroy(screen->data, state);
    }
    free(screen);
}

bool screen_update(struct screen *screen, struct state *state, struct env *env) {
    return screen->impl->update(screen->data, state, env);
}
