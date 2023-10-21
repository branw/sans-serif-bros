#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <memory.h>
#include "canvas.h"
#include "util.h"
#include "log.h"

void canvas_create(struct canvas *canvas, unsigned w, unsigned h) {
    // Allocate the two buffers
    canvas->buf[0] = calloc(w * h, sizeof(struct cell));
    canvas->buf[1] = calloc(w * h, sizeof(struct cell));
    canvas->w = w, canvas->h = h;

    // Initialize the starting style
    canvas_reset(canvas);

    // Reset the flushing parameters
    canvas->force_flush = canvas->force_next_flush_only = false;
    canvas->flush_index = 0;
    CANVAS_CELL_CLEAR(canvas->flush_state);

    // For the sake of consistency, all blank cells store spaces
    struct cell empty_cell = canvas->style, space_cell = canvas->style;
    space_cell.code_point = ' ';
    for (unsigned y = 0; y < h; y++) {
        for (unsigned x = 0; x < w; x++) {
            canvas->buf[0][x + y * w] = empty_cell;
            canvas->buf[1][x + y * w] = space_cell;
        }
    }
}

void canvas_destroy(struct canvas *canvas) {
    free(canvas->buf[0]);
    free(canvas->buf[1]);
}

void canvas_resize(struct canvas *canvas, unsigned w, unsigned h) {
    if (canvas->w == w && canvas->h == h) {
        return;
    }

    // Copy the existing canvas and initialize a new one
    struct canvas old = *canvas;
    canvas_create(canvas, w, h);

    // Copy the existing second buffer over
    unsigned copy_w = (old.w > w) ? w : old.w, copy_h = (old.h > h) ? h : old.h;
    for (unsigned y = 0; y < copy_h; y++) {
        memcpy(canvas->buf[1], old.buf[1], copy_w * sizeof(struct cell));
    }

    // Free the existing canvas
    canvas_destroy(&old);

    // Interrupt the current flush
    canvas->flush_index = 0;
    canvas_force_next_flush(canvas);
}

void canvas_erase(struct canvas *canvas) {
    unsigned x1 = 0, y1 = 0, x2 = x1 + canvas->w, y2 = y1 + canvas->h;

    struct cell new_cell = canvas->style;
    new_cell.code_point = ' ';

    for (unsigned y = y1; y < y2; y++) {
        for (unsigned x = x1; x < x2; x++) {
            canvas->buf[1][x + y * canvas->w] = new_cell;
        }
    }
}

#define CANVAS_LEN (canvas->w * canvas->h)

bool canvas_flush(struct canvas *canvas, char *buf, size_t len, size_t *len_written) {
    if (canvas->flush_index == CANVAS_LEN) {
        if (canvas->force_flush && canvas->force_next_flush_only) {
            canvas->force_flush = canvas->force_next_flush_only = false;
        }

        return false;
    }

    bool early_exit = false;
    size_t index = canvas->flush_index, last_index = index, remaining = len;
    while (remaining > 0 && index < CANVAS_LEN) {
        struct cell prev = canvas->buf[0][index], next = canvas->buf[1][index];

        // Ignore unchanged cells at the cost of a cursor move which may
        // potentially be longer
        if (!canvas->force_flush && CANVAS_CELL_EQ(prev, next)) {
            index++;
            continue;
        }

        // There's a chance that part of this message will get cut off. To
        // reduce complexity, we simply discard the output for an entire cell if
        // a single part of it won't fit
        size_t initial_remaining = remaining;

        // Move the cursor explicitly when the previous block wasn't changed or
        // was on a different line
        bool noncontinuous = (last_index + 1 != index),
                newline = (index % canvas->w == 0);
        if (noncontinuous || newline) {
            int x = (int) (index % canvas->w), y = (int) (index / canvas->w);
            int escape_len = snprintf(buf, remaining, "\x1b[%d;%dH", y + 1, x + 1);
            if (escape_len < 0) {
                remaining = initial_remaining;
                break;
            }
            else if (escape_len >= remaining) {
                remaining = initial_remaining;
                early_exit = true;
                break;
            }

            buf += escape_len;
            remaining -= escape_len;
        }

        // Emit styling if it's different from the last flushed character
        if (!CANVAS_CELL_STYLE_EQ(canvas->flush_state, next)) {
            int escape_len = snprintf(buf, remaining, "\x1b[%d;%dm",
                                      (int)next.foreground + 30,(int)next.background + 40);
            if (escape_len < 0) {
                remaining = initial_remaining;
            }
            else if (escape_len >= remaining) {
                remaining = initial_remaining;
                early_exit = true;
                break;
            }

            buf += escape_len;
            remaining -= escape_len;

            // Save the last flushed cell style
            canvas->flush_state = next;
        }

        // Now try to emit the actual character
        size_t encoded_len = utf8_encode(next.code_point, &buf, remaining);
        if (encoded_len > remaining) {
            remaining = initial_remaining;
            early_exit = true;
            break;
        }

        remaining -= encoded_len;

        // Mark the cell as clean
        canvas->buf[0][index] = next;

        last_index = index++;
    }

    canvas->flush_index = index;

    *len_written = len - remaining;
    if (!early_exit && *len_written > 0) {
        return true;
    }

    return early_exit;
}

bool canvas_forced_flush(struct canvas *canvas, char *buf, size_t len, size_t *len_written) {
    canvas->force_flush = true;
    bool res = canvas_flush(canvas, buf, len, len_written);
    canvas->force_flush = false;
    return res;
}

void canvas_force_next_flush(struct canvas *canvas) {
    canvas->force_flush = true;
    canvas->force_next_flush_only = true;
}

void canvas_write(struct canvas *canvas, unsigned x, unsigned y, char *msg) {
    unsigned len = strlen(msg);
    ASSERT(y + len <= canvas->w);

    struct cell *cell = &canvas->buf[1][x + y * canvas->w];
    for (unsigned i = 0; i < len; i++, cell++, msg++) {
        struct cell new_cell = canvas->style;
        new_cell.code_point = (uint8_t)*msg;
        *cell = new_cell;
    }
}

void canvas_write_block(struct canvas *canvas, unsigned x1, unsigned y1, unsigned w,
                              unsigned h, char *buf) {
    ASSERT(x1 + w <= canvas->w);
    ASSERT(y1 + h <= canvas->h);
    ASSERT(strlen(buf) <= w * h);

    for (unsigned y = y1; y < y1 + h && *buf != '\0'; y++) {
        struct cell *cell = &canvas->buf[1][x1 + y * canvas->w];
        for (unsigned x = x1; x < x1 + w && *buf != '\0'; x++, buf++, cell++) {
            struct cell new_cell = canvas->style;
            new_cell.code_point = (uint8_t)*buf;
            *cell = new_cell;
        }
    }

    canvas->flush_index = 0;
}

void canvas_write_utf8(struct canvas *canvas, unsigned x, unsigned y, char *msg) {
    ASSERT(false);

    /*
    struct canvas *window = canvas;
    if (canvas->window) {
        canvas = canvas->window;
    }

    canvas->flush_index = 0;

    //TODO decode utf8
     */
}

void canvas_write_block_utf32(struct canvas *canvas, unsigned x1, unsigned y1, unsigned w,
                              unsigned h, uint32_t *buf, size_t len) {
    ASSERT(x1 + w <= canvas->w);
    ASSERT(y1 + h <= canvas->h);

    for (unsigned y = y1; y < y1 + h; y++) {
        struct cell *cell = &canvas->buf[1][x1 + y * canvas->w];
        for (unsigned x = x1; x < x1 + w; x++, buf++, cell++) {
            struct cell new_cell = canvas->style;
            new_cell.code_point = *buf;
            *cell = new_cell;
        }
    }

    canvas->flush_index = 0;
}

void canvas_put(struct canvas *canvas, unsigned x, unsigned y, unsigned long c) {
    ASSERT(x < canvas->w);
    ASSERT(y < canvas->h);

    struct cell new_cell = canvas->style;
    new_cell.code_point = c;
    canvas->buf[1][x + y * canvas->w] = new_cell;

    canvas->flush_index = 0;
}

unsigned long canvas_get(struct canvas *canvas, unsigned x, unsigned y) {
    return canvas->buf[1][x + y * canvas->w].code_point;
}

void canvas_fill(struct canvas *canvas, unsigned x, unsigned y, unsigned w, unsigned h,
                 unsigned long symbol) {
    ASSERT(x + w <= canvas->w);
    ASSERT(y + h <= canvas->h);

    struct cell new_cell = canvas->style;
    new_cell.code_point = symbol;

    for (unsigned row = y; row < y + h; row++) {
        for (unsigned col = x; col < x + w; col++) {
            canvas->buf[1][col + row * canvas->w] = new_cell;
        }
    }

    canvas->flush_index = 0;
}

void canvas_rect(struct canvas *canvas, unsigned x, unsigned y, unsigned w, unsigned h,
                 unsigned long symbol) {
    ASSERT(x + w <= canvas->w);
    ASSERT(y + h <= canvas->h);

    struct cell new_cell = canvas->style;
    new_cell.code_point = symbol;

    for (unsigned row = y; row < y + h; row++) {
        unsigned const step = (row == y || row == y + h - 1 || w == 1) ? 1 : w - 1;
        for (unsigned col = x; col < x + w; col += step) {
            canvas->buf[1][col + row * canvas->w] = new_cell;
        }
    }

    canvas->flush_index = 0;
}

void canvas_line(struct canvas *canvas, unsigned x0, unsigned y0, unsigned x1, unsigned y1,
                 unsigned long symbol) {
    ASSERT(x0 <= canvas->w);
    ASSERT(x1 <= canvas->w);
    ASSERT(y0 <= canvas->h);
    ASSERT(y1 <= canvas->h);

    struct cell new_cell = canvas->style;
    new_cell.code_point = symbol;

    // Bresenham's line algorithm
    // From https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
    int const dx = abs((int)x0 - (int)x1);
    int const sx = x0 < x1 ? 1 : -1;
    int const dy = -abs((int)y0 - (int)y1);
    int const sy = y0 < y1 ? 1 : -1;

    int x = (int)x0;
    int y = (int)y0;
    int error = dx + dy;
    for (;;) {
        canvas->buf[1][x + y * canvas->w] = new_cell;
        if (x == x1 && y == y1) {
            break;
        }

        int const e2 = error * 2;
        if (e2 >= dy) {
            if (x == x1) {
                break;
            }
            error += dy;
            x += sx;
        }
        if (e2 <= dx) {
            if (y == y1) {
                break;
            }
            error += dx;
            y += sy;
        }
    }

    canvas->flush_index = 0;
}

void canvas_reset(struct canvas *canvas) {
    CANVAS_CELL_CLEAR(canvas->style);
    canvas->style.background = default_color;
    canvas->style.foreground = default_color;
}

//void canvas_italic(struct canvas *canvas, bool state) {
//    canvas->style.italic = state ? true : false;
//}
//
//void canvas_underline(struct canvas *canvas, bool state) {
//    canvas->style.underline = state ? true : false;
//}
//
//void canvas_blink(struct canvas *canvas, bool state) {
//    canvas->style.blink = state ? true : false;
//}

void canvas_bold(struct canvas *canvas, bool state) {
    canvas->style.bold = state ? true : false;
}

void canvas_foreground(struct canvas *canvas, enum color color) {
    canvas->style.foreground = color;
}

void canvas_background(struct canvas *canvas, enum color color) {
    canvas->style.background = color;
}
