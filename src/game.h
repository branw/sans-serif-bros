#ifndef SSB_GAME_H
#define SSB_GAME_H

#include <stdint.h>
#include "terminal.h"
#include "config.h"

struct game {
    unsigned tick;

    bool win, die, no_money_left;
    bool reverse;
    int tired;

    uint32_t field[ROWS][COLUMNS];
    uint32_t next_field[ROWS][COLUMNS];

    struct directional_input input;
};

void game_create(struct game *game, uint32_t *stage);

void game_create_from_utf8(struct game *game, char *stage);

void game_update(struct game *game, struct directional_input *input);

#endif //SSB_GAME_H
