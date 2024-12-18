#ifndef SSB_GAME_H
#define SSB_GAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "terminal.h"
#include "config.h"

enum game_input {
    GAME_NO_INPUT,
    GAME_LEFT_INPUT,
    GAME_RIGHT_INPUT,
    GAME_UP_INPUT,
    GAME_DOWN_INPUT,
};

struct game {
    unsigned tick;

    bool win, die, no_money_left;
    bool reverse;
    int tired;

    uint32_t field[ROWS][COLUMNS];
    uint32_t next_field[ROWS][COLUMNS];

    enum game_input last_input;
    uint32_t ticks_since_input_started;

    char input_log[INPUT_LOG_LEN];
    size_t input_log_len;

    struct directional_input input;
};

enum game_state {
    GAME_STATE_IN_PROGRESS = 0,
    GAME_STATE_WON = 1,
    GAME_STATE_DIED,
    GAME_STATE_QUIT,
    GAME_STATE_RETRIED,
};

bool game_parse_and_validate_field(char *field_str, uint32_t *field);

bool game_create_from_utf8(struct game *game, char *stage);

enum game_state game_update(struct game *game, struct directional_input *input);

char const *game_state_to_str(enum game_state game_state);

#ifdef __cplusplus
}
#endif

#endif //SSB_GAME_H
