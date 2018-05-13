#ifndef SSB_STATE_H
#define SSB_STATE_H

#include <stdbool.h>

#include "terminal.h"

enum screen {
    title_screen,
    playing_screen,
    pause_screen
};

struct state {
    enum screen screen;

    enum parse_state parse_state;
};

struct session;

bool state_update(struct session *sess);

#endif //SSB_STATE_H
