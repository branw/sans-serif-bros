#ifndef SSB_GAME_H
#define SSB_GAME_H

#include "terminal.h"
#include "config.h"

struct game_state {
    unsigned tick;

    bool win, die, no_money_left;
    bool reverse;
    int tired;

    unsigned long field[ROWS][COLUMNS];
    unsigned long next_field[ROWS][COLUMNS];

    struct menu_input input;
};

void game_init(struct game_state *state);

void game_update(struct game_state *state, struct menu_input *input);

#endif //SSB_GAME_H
