#include <memory.h>
#include <stdio.h>
#include <baro.h>
#include "game.h"
#include "util.h"
#include "log.h"

#define MONEY 0xa3
#define PIPE 0xa6

#define UP (state->input.up)
#define DOWN (state->input.down)
#define LEFT (state->input.left)
#define RIGHT (state->input.right)

bool game_parse_and_validate_field(char *field_str, uint32_t *field) {
    int row = 0;
    int column = 0;
    while (*field_str != '\0') {
        uint32_t code_point = utf8_decode(&field_str);
        if (code_point == '\n') {
            column = 0;
            row += 1;
        } else if (code_point == '\r') {
            continue;
        } else {
            if (row >= ROWS) {
                LOG_ERROR("Bad level: too many rows");
                return false;
            } else if (column >= COLUMNS) {
                LOG_ERROR("Bad level: too many columns in row %d", row);
                return false;
            }

            field[row * COLUMNS + column] = code_point;
            column += 1;
        }
    }

    return true;
}

TEST("[game] game_parse_and_validate_field") {
    uint32_t field[COLUMNS * ROWS] = {0};

    REQUIRE(game_parse_and_validate_field("", field));
    REQUIRE(game_parse_and_validate_field("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n \n", field));
    REQUIRE_FALSE(game_parse_and_validate_field("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n \n", field));
}

bool game_create_from_utf8(struct game *game, char *field_str) {
    game->tick = 0;
    game->win = game->die = game->no_money_left = false;
    game->tired = 0;

    game->last_input = GAME_NO_INPUT;
    game->ticks_since_input_started = 0;

    memset(game->input_log, 0, sizeof(game->input_log));
    game->input_log_len = 0;

    return game_parse_and_validate_field(field_str, (uint32_t *) game->field);
}

static void replace(struct game *state, unsigned long from, unsigned long to) {
    for (unsigned y = 0; y < ROWS; ++y) {
        for (unsigned x = 0; x < COLUMNS; ++x) {
            if (state->field[y][x] == from) {
                state->next_field[y][x] = to;
            }
        }
    }
}

static void swap(struct game *state, unsigned long a, unsigned long b) {
    for (unsigned y = 0; y < ROWS; ++y) {
        for (unsigned x = 0; x < COLUMNS; ++x) {
            if (state->field[y][x] == a) {
                state->next_field[y][x] = b;
                state->field[y][x] = ' ';
            } else if (state->field[y][x] == b) {
                state->next_field[y][x] = a;
                state->field[y][x] = ' ';
            }
        }
    }
}

static bool probe(struct game *state, unsigned x, unsigned y, unsigned long ch) {
    if (y >= ROWS || x >= COLUMNS) {
        return true;
    }

    unsigned long const ob = state->field[y][x], next_ob = state->next_field[y][x];

    switch (ch) {
    case 'I': {
        switch (ob) {
        case 'E': {
            state->win = true;
            return false;
        }

        case MONEY: {
            state->next_field[y][x] = ' ';
            return false;
        }

        case '0': {
            return true;
        }

        case '[':
        case ']':
        case '{':
        case '}':
        case '%': {
            state->die = true;
            return true;
        }

        case 'O': {
            int const d = LEFT ? -1 : (RIGHT ? 1 : 0);
            if (d != 0 && probe(state, x, y + 1, ob) && !probe(state, x + d, y, ob) &&
                state->field[y][x - d] == 'I') {
                state->next_field[y][x + d] = ob;
                state->next_field[y][x] = ' ';

                if (state->field[y + 1][x] == ';') {
                    state->next_field[y + 2][x] = ' ';
                }

                if (!state->tired) {
                    state->tired = 1;
                }
                return false;
            }
            return true;
        }

        default:
            break;
        }

        break;
    }

    case '<':
    case '>': {
        if (ob == 'I') {
            return false;
        }
        break;
    }

    case '{':
    case '}':
    case '[':
    case ']': {
        switch (ob) {
        case 'O':
        case MONEY: {
            int d = (ch == '}' || ch == ']') ? 1 : -1;
            if (probe(state, x, y + 1, ob) && !probe(state, x + d, y, ob)) {
                state->next_field[y][x + d] = ob;
                return false;
            }
            break;
        }

        case 'I': {
            state->die = true;
            return false;
        }

        default:
            break;
        }
        break;
    }

    default:
        break;
    }

    return !(ob == ' ' && next_ob == ' ');
}

static void process_frame_1(struct game *state) {
    int money_left = 0;
    int players_left = 0;

    for (unsigned y = 0; y < ROWS; ++y) {
        for (unsigned x = 0; x < COLUMNS; ++x) {
            unsigned long ch = state->field[y][x];

            if ((ch == 'I' || ch == '[' || ch == ']' || ch == 'O' || ch == '%' || ch == MONEY) &&
                y == ROWS - 1) {
                state->next_field[y][x] = ' ';
            } else if (ch >= '1' && ch <= '9' && y > 0 && state->field[y - 1][x] != ' ') {
                state->next_field[y][x] = ch - 1;
            } else {
                switch (ch) {
                case '0': {
                    state->next_field[y][x] = ' ';
                    break;
                }

                case '%': {
                    if (y < ROWS - 1 && state->field[y + 1][x] == ';' && state->next_field[y][x] ==
                                                                         '%') {
                        state->next_field[y][x] = ' ';
                        if (y < ROWS - 2) {
                            state->next_field[y + 2][x] = ch;
                        }
                    } else if (!probe(state, x, y + 1, ch) ||
                               (y < ROWS - 1 && state->field[y + 1][x] == 'I')) {
                        state->next_field[y + 1][x] = ch;
                        state->next_field[y][x] = ' ';
                    }
                    break;
                }

                case ':': {
                    if (y > 0) {
                        if (state->field[y - 1][x] == 'O' || state->field[y - 1][x] == '%') {
                            state->next_field[y][x] = ';';
                        } else if (state->field[y - 1][x] == 'X' || state->field[y - 1][x] == '.') {
                            state->next_field[y][x] = '.';
                        }
                    }
                    break;
                }

                case ';': {
                    if (y > 0 && state->field[y - 1][x] != 'O' && state->field[y - 1][x] != '%') {
                        state->next_field[y][x] = ':';
                    }
                    break;
                }

                case 'O': {
                    if (y < ROWS - 1 &&
                        state->field[y + 1][x] == ';' && state->next_field[y][x] == 'O') {
                        state->next_field[y][x] = ' ';
                        if (y < ROWS - 2) {
                            state->next_field[y + 2][x] = ch;
                        }
                    } else if (!probe(state, x, y + 1, ch)) {
                        state->next_field[y + 1][x] = ch;
                        state->field[y][x] = ' ';
                        state->next_field[y][x] = ' ';
                    }
                    break;
                }

                case '.': {
                    state->next_field[y][x] = ':';
                    break;
                }

                case '&':
                case '?': {
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            if ((x + dx) >= 0 && (y + dx) >= 0 &&
                                (x + dx) <= COLUMNS - 1 && (y + dy) <= ROWS - 1 &&
                                state->field[y + dy][x + dx] == '0') {
                                state->next_field[y][x] = '0';
                            }
                        }
                    }
                    break;
                }

                case MONEY: {
                    ++money_left;

                    if (!probe(state, x, y + 1, ch)) {
                        state->next_field[y + 1][x] = ch;
                        state->next_field[y][x] = ' ';
                    } else if (y < ROWS - 1 && state->field[y + 1][x] == 'I') {
                        state->next_field[y][x] = ' ';
                    }
                    break;
                }

                case 'T': {
                    if (y > 0 && state->field[y - 1][x] == 'I') {
                        if (x > 0 && LEFT &&
                            state->next_field[y - 1][x - 1] == 'I' && !probe(state, x - 1, y, ch)) {
                            state->next_field[y][x - 1] = ch;
                            state->next_field[y][x] = ' ';
                        } else if (x < COLUMNS - 1 && RIGHT &&
                                   state->next_field[y - 1][x + 1] == 'I' &&
                                   !probe(state, x + 1, y, ch)) {
                            state->next_field[y][x + 1] = ch;
                            state->next_field[y][x] = ' ';
                        }
                    }
                    break;
                }

                case PIPE: {
                    if (y > 0 && state->field[y - 1][x] == '.') {
                        state->next_field[y][x] = 'A';
                    }
                    break;
                }

                case 'A': {
                    if (y > 0 && (state->field[y - 1][x] == ':' || state->field[y - 1][x] == '.')
                        && !probe(state, x, y + 1, 'O')) {
                        state->next_field[y + 1][x] = 'O';
                        state->next_field[y][x] = PIPE;
                    }
                    break;
                }

                case 'I': {
                    ++players_left;

                    if (!probe(state, x, y + 1, ch)) {
                        state->next_field[y + 1][x] = ch;
                        state->next_field[y][x] = ' ';
                    } else if (y < ROWS - 1) {
                        unsigned long fl = state->field[y + 1][x];

                        if (fl == '(' || fl == ')' || state->tired) {
                            break;
                        }

                        if (LEFT && x > 0) {
                            if (!probe(state, x - 1, y, ch)) {
                                state->next_field[y][x - 1] = ch;
                                state->next_field[y][x] = ' ';
                            } else if (y > 0 && !probe(state, x - 1, y - 1, ch)) {
                                state->next_field[y - 1][x - 1] = ch;
                                state->next_field[y][x] = ' ';
                            }
                        } else if (RIGHT && x < COLUMNS - 1) {
                            if (!probe(state, x + 1, y, ch)) {
                                state->next_field[y][x + 1] = ch;
                                state->next_field[y][x] = ' ';
                            } else if (y > 0 && !probe(state, x + 1, y - 1, ch)) {
                                state->next_field[y - 1][x + 1] = ch;
                                state->next_field[y][x] = ' ';
                            }
                        } else if (UP) {
                            if (y > 0 && state->field[y - 1][x] == '-' &&
                                !probe(state, x, y - 2, ch)) {
                                state->next_field[y - 2][x] = 'I';
                                state->next_field[y][x] = ' ';
                            } else if (y > 0 && y < ROWS - 1 && state->field[y + 1][x] == '"' &&
                                       !probe(state, x, y - 1, ch)) {
                                state->next_field[y + 1][x] = ' ';
                                state->next_field[y][x] = '"';
                                state->next_field[y - 1][x] = ch;
                            }
                        } else if (DOWN) {
                            if (y < ROWS - 1 && state->field[y + 1][x] == '-' &&
                                !probe(state, x, y + 2, ch)) {
                                state->next_field[y + 2][x] = 'I';
                                state->next_field[y][x] = ' ';
                            } else if (y < ROWS - 1 && state->field[y + 1][x] == '"' &&
                                       !probe(state, x, y + 2, '"')) {
                                state->next_field[y + 2][x] = '"';
                                state->next_field[y + 1][x] = ch;
                                state->next_field[y][x] = ' ';
                            } else if (y < ROWS - 1 && state->field[y + 1][x] == '~') {
                                replace(state, '@', '0');
                            } else if (y < ROWS - 1 && state->field[y + 1][x] == '`') {
                                state->reverse = true;
                            }
                        }
                    }
                    break;
                }

                case 'x': {
                    if (y > 0 && state->field[y - 1][x] != ' ') {
                        state->next_field[y - 1][x] = ' ';
                        state->next_field[y][x] = 'X';
                    }
                    break;
                }

                case 'X': {
                    state->next_field[y][x] = 'x';
                    break;
                }

                case 'e': {
                    if (state->no_money_left) {
                        state->next_field[y][x] = 'E';
                    }
                    break;
                }

                case 'E': {
                    if (!state->no_money_left) {
                        state->next_field[y][x] = 'e';
                    }
                    break;
                }

                case '(':
                case ')': {
                    if (y > 0) {
                        unsigned long ob = state->field[y - 1][x];
                        int d = (ch == ')') ? 1 : -1;
                        if ((ob == 'I' || ob == '[' || ob == ']' || ob == 'O' || ob == '%' ||
                             ob == MONEY) && !probe(state, x + d, y - 1, ob)) {
                            state->next_field[y - 1][x] = ' ';
                            state->next_field[y - 1][x + d] = ob;
                        }
                    }
                    break;
                }

                case '<':
                case '>': {
                    int d = (ch == '<') ? -1 : 1;
                    if (!probe(state, x + d, y, ch)) {
                        state->next_field[y][x] = ' ';
                        state->next_field[y][x + d] = ch;

                        if (y > 0) {
                            unsigned long ob = state->field[y - 1][x];
                            if ((ob == 'I' || ob == '[' || ob == ']' || ob == 'O' || ob == '%' ||
                                 ob == MONEY) && !probe(state, x + d, y - 1, ob)) {
                                state->next_field[y - 1][x] = ' ';
                                state->next_field[y - 1][x + d] = ob;
                            }
                        }
                    }
                    break;
                }

                case '{':
                case '}':
                case '[':
                case ']': {
                    if (y < ROWS - 1) {
                        int d = (ch == '[' || ch == '{') ? -1 : 1;
                        bool gr = (ch == '[' || ch == ']');
                        unsigned long od = (unsigned char) ((d > 0) ? (gr ? '[' : '{') : (gr ? ']' : '}'));

                        unsigned long fl = state->field[y + 1][x];
                        if (!(gr && (fl == '(' || fl == ')'))) {
                            if (probe(state, x + d, y, ch) && (!gr || probe(state, x, y + 1, ch))) {
                                state->next_field[y][x] = od;
                            } else if (gr && !probe(state, x, y + 1, ch)) {
                                state->next_field[y][x] = ' ';
                                state->next_field[y + 1][x] = ch;
                            } else {
                                state->next_field[y][x] = ' ';
                                state->next_field[y][x + d] = ch;
                            }
                        }
                    }
                    break;
                }

                default:
                    break;
                }
            }
        }
    }

    if (money_left == 0) {
        state->no_money_left = true;
    }
    if (players_left == 0) {
        state->die = true;
    }
}

static void process_frame_8(struct game *state) {
    for (unsigned y = 0; y < ROWS; ++y) {
        for (unsigned x = 0; x < COLUMNS; ++x) {
            unsigned long ch = state->field[y][x];
            switch (ch) {
            case '=': {
                if (y == 0) {
                    continue;
                }

                unsigned long ob = state->field[y - 1][x];
                if (ob != ' ' && ob != 'I' && !probe(state, x, y + 1, ob)) {
                    state->next_field[y + 1][x] = ob;
                }
                break;
            }

            case 'b':
            case 'd': {
                int d = (ch == 'd') ? -1 : 1;
                if (probe(state, x + d, y, ch)) {
                    state->next_field[y][x] = (uint8_t) ((ch == 'd') ? 'b' : 'd');
                } else {
                    state->next_field[y][x] = ' ';
                    state->next_field[y][x + d] = ch;
                }
                break;
            }

            default:
                break;
            }
        }
    }
}

static char const GAME_INPUT_TO_CHAR[] = {
        [GAME_NO_INPUT] = 'I',
        [GAME_LEFT_INPUT] = 'L',
        [GAME_RIGHT_INPUT] = 'R',
        [GAME_UP_INPUT] = 'U',
        [GAME_DOWN_INPUT] = 'D',
};

enum game_state game_update(struct game *game, struct directional_input *input) {
    // Exit early if the game is already over to avoid mutating the state
    if (game->win) {
        return GAME_STATE_WON;
    } else if (game->die) {
        return GAME_STATE_DIED;
    }

    // Process input
    game->input = *input;

    // Append input to the log
    enum game_input const game_input =
            input->left ? GAME_LEFT_INPUT :
            input->right ? GAME_RIGHT_INPUT :
            input->up ? GAME_UP_INPUT :
            input->down ? GAME_DOWN_INPUT :
            GAME_NO_INPUT;
    if (game->last_input == game_input) {
        game->ticks_since_input_started++;
    } else if (game->input_log_len < INPUT_LOG_LEN) {
        char const input_char = GAME_INPUT_TO_CHAR[game->last_input];

        // If the last input was played for multiple ticks, log the number of
        // ticks and then a symbol for the input
        if (game->ticks_since_input_started > 1) {
            game->input_log_len += snprintf(
                    game->input_log + game->input_log_len,
                    INPUT_LOG_LEN - game->input_log_len,
                    "%d%c",
                    game->ticks_since_input_started,
                    input_char);
        }
        // If the last input was only used for one tick, just output the symbol
        else {
            game->input_log[game->input_log_len++] = input_char;
        }

        if (game->input_log_len >= INPUT_LOG_LEN) {
            LOG_ERROR("Outgrew input log (%d bytes)!", game->input_log_len);
        }

        game->last_input = game_input;
        game->ticks_since_input_started = 1;
    }

    // Process the game field
    memcpy(game->next_field, game->field, ROWS * COLUMNS * sizeof(uint32_t));

    if (game->reverse) {
        swap(game, '{', '}');
        swap(game, '[', ']');
        swap(game, '<', '>');
        swap(game, '(', ')');
        game->reverse = false;
    }

    process_frame_1(game);
    if (game->tick++ % 8 == 7) {
        process_frame_8(game);
    }

    if (game->win || game->die) {
        LOG_DEBUG("Input log (%d ticks, %d bytes): %s", game->tick, game->input_log_len, game->input_log);
    }

    memcpy(game->field, game->next_field, ROWS * COLUMNS * sizeof(uint32_t));

    if (game->tired) {
        ++game->tired;
        if (game->tired > 2) {
            game->tired = 0;
        }
    }

    return GAME_STATE_IN_PROGRESS;
}

TEST("[game] update") {

}

char const *game_state_to_str(enum game_state game_state) {
    switch (game_state) {
        case GAME_STATE_IN_PROGRESS: return "in_progress";
        case GAME_STATE_WON: return "won";
        case GAME_STATE_DIED: return "died";
        case GAME_STATE_QUIT: return "quit";
        case GAME_STATE_RETRIED: return "retried";
        default: return "<unknown game_state>";
    }
}
