#ifndef SSB_GAME_H
#define SSB_GAME_H

#include "terminal.h"

struct game_state {
    unsigned tick;

    bool win, die, no_money_left;
    bool reverse;
    int tired;

    char field[25][80];
    char next_field[25][80];

    struct menu_input input;
};

void game_init(struct game_state *state);

void game_update(struct game_state *state);

#endif //SSB_GAME_H
