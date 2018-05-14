#ifndef SSB_STATE_H
#define SSB_STATE_H

#include <stdbool.h>
#include <time.h>

#include "terminal.h"

// Duration in ms between updates
#define TICK_DURATION 125

// Number of bytes to store for each session; must be a power of 2
#define INPUT_BUF_LEN 512

enum screen {
    title_screen
};

struct state {
    enum screen screen;

    struct timespec last_tick;

    char input_buf[INPUT_BUF_LEN];
    unsigned input_buf_read, input_buf_write;
};

struct session;

void state_init(struct session *sess);

bool state_update(struct session *sess);

#endif //SSB_STATE_H
