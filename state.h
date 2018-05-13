#ifndef SSB_STATE_H
#define SSB_STATE_H

#include <stdbool.h>

#include "terminal.h"

enum screen {
    title_screen
};

struct state {
    enum screen screen;
};

struct session;

void state_init(struct session *sess);

bool state_update(struct session *sess);

#endif //SSB_STATE_H
