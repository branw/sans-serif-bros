#ifndef SSB_STATE_H
#define SSB_STATE_H

#include <stdbool.h>
#include <time.h>

#include "terminal.h"
#include "game.h"
#include "canvas.h"

enum screen {
    title_screen, game_screen
};

struct state {
    struct timespec last_tick;
    enum screen screen;

    struct canvas canvas;

    struct terminal_state terminal_state;

    struct game_state game_state;
};

struct session;

void state_init(struct session *sess);

bool state_update(struct session *sess);

#endif //SSB_STATE_H
