#ifndef SSB_GAME_H
#define SSB_GAME_H

#ifdef __cplusplus
extern "C" {
#endif

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

bool game_parse_and_validate_field(char *field_str, uint32_t *field);

bool game_create_from_utf8(struct game *game, char *stage);

void game_update(struct game *game, struct directional_input *input);

#ifdef __cplusplus
}
#endif

#endif //SSB_GAME_H
