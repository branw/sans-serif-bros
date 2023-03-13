#include <memory.h>
#include "game.h"
#include "util.h"

#define MONEY 0xa3
#define PIPE 0xa6

#define UP (state->input.up)
#define DOWN (state->input.down)
#define LEFT (state->input.left)
#define RIGHT (state->input.right)

void game_create(struct game *game, uint32_t *stage) {
    game->tick = 0;
    game->win = game->die = game->no_money_left = false;
    game->tired = 0;

    memcpy(game->field, stage, ROWS * COLUMNS * sizeof(uint32_t));
}

void game_create_from_utf8(struct game *game, char *stage) {
    game->tick = 0;
    game->win = game->die = game->no_money_left = false;
    game->tired = 0;

    unsigned long *field = (unsigned long *) game->field;
    while (*stage) {
        *field++ = utf8_decode(&stage);
    }
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

    unsigned long ob = state->field[y][x], next_ob = state->next_field[y][x];

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
            int d = state->input.left ? -1 : (state->input.right ? 1 : 0);
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

void game_update(struct game *game, struct directional_input *input) {
    game->input = *input;

    if (game->win || game->die) {
        return;
    }

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

    memcpy(game->field, game->next_field, ROWS * COLUMNS * sizeof(uint32_t));

    if (game->tired) {
        ++game->tired;
        if (game->tired > 2) {
            game->tired = 0;
        }
    }
}
