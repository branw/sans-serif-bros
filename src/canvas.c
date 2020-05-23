#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <memory.h>
#include "canvas.h"
#include "util.h"

void canvas_create(struct canvas *canvas, unsigned w, unsigned h) {
    // Allocate the two buffers
    canvas->buf[0] = calloc(w * h, sizeof(struct cell));
    canvas->buf[1] = calloc(w * h, sizeof(struct cell));
    canvas->w = w, canvas->h = h;

    // Initialize the starting style
    canvas_reset(canvas);

    // Reset the flushing parameters
    canvas->force_flush = false;
    canvas->flush_index = 0;
    CANVAS_CELL_CLEAR(canvas->flush_state);
    canvas->flush_encode_offset = 0;
    canvas->flush_last_index = 0;

    // For the sake of consistency, all blank cells store spaces
    struct cell new_cell = canvas->style;
    new_cell.code_point = ' ';
    for (unsigned y = 0; y < h; y++) {
        for (unsigned x = 0; x < w; x++) {
            canvas->buf[0][x + y * w] = canvas->buf[1][x + y * w] = new_cell;
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
        return false;
    }

    size_t index = canvas->flush_index, remaining = len;
    while (remaining > 0 && index < CANVAS_LEN) {
        struct cell prev = canvas->buf[0][index], next = canvas->buf[1][index];

        // Ignore unchanged cells at the cost of a cursor move which may
        // potentially be longer
        if (!canvas->force_flush && CANVAS_CELL_EQ(prev, next)) {
            index++;
            continue;
        }

        // Move the cursor explicitly when the previous block wasn't changed or
        // was on a different line
        bool noncontinuous = (canvas->flush_last_index + 1 != index),
                newline = (index % canvas->w == 0);
        if (noncontinuous || newline) {
            int x = (int) (index % canvas->w), y = (int) (index / canvas->w);
            int escape_len = snprintf(buf, remaining, "\x1b[%d;%dH", y + 1, x + 1);

            buf += escape_len;
            remaining -= escape_len;
        }

        canvas->flush_last_index = index;

        //TODO allow for split escape sequences

        //TODO parse state and emit escape sequences

        // Now try to emit the actual character
        size_t encoded_len = utf8_encode(canvas->flush_encode_offset, &buf, remaining,
                                         next.code_point);

        // The encoded character didn't entirely fit, so we'll need to send the
        // rest next flush
        if (encoded_len > remaining) {
            canvas->flush_encode_offset = encoded_len - remaining;
            remaining = 0;
            break;
        }

        canvas->flush_encode_offset = 0;
        remaining -= encoded_len;

        // Mark the cell as clean
        canvas->buf[0][index] = next;

        index++;
    }

    canvas->flush_index = index;
    *len_written = len - remaining;
    return *len_written > 0;
}

bool canvas_forced_flush(struct canvas *canvas, char *buf, size_t len, size_t *len_written) {
    canvas->force_flush = true;
    bool res = canvas_flush(canvas, buf, len, len_written);
    canvas->force_flush = false;
    return res;
}

void canvas_write_utf8(struct canvas *canvas, unsigned x, unsigned y, char *msg) {
    assert(false);

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
                              unsigned h, unsigned long *buf, size_t len) {
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
    assert(x < canvas->w);
    assert(y < canvas->h);

    struct cell new_cell = canvas->style;
    new_cell.code_point = c;
    canvas->buf[1][x + y * canvas->w] = new_cell;

    canvas->flush_index = 0;
}

unsigned long canvas_get(struct canvas *canvas, unsigned x, unsigned y) {
    return canvas->buf[1][x + y * canvas->w].code_point;
}

void canvas_rect(struct canvas *canvas, unsigned x, unsigned y, unsigned w, unsigned h,
                 unsigned long symbol) {
    assert(x + w <= canvas->w);
    assert(y + h <= canvas->h);

    struct cell new_cell = canvas->style;
    new_cell.code_point = symbol;

    for (unsigned row = y; row < y + h; row++) {
        int step = (row == y || row == y + h - 1) ? 1 : w - 1;
        for (unsigned col = x; col < x + w; col += step) {
            canvas->buf[1][col + row * canvas->w] = new_cell;
        }
    }
}

void canvas_reset(struct canvas *canvas) {
    // 3-bit colors means that we can't represent the default color and instead
    // must create our own; this at least gives some consistency across themes
    CANVAS_CELL_CLEAR(canvas->style);
    canvas->style.background = black;
    canvas->style.foreground = white;
}

void canvas_italic(struct canvas *canvas, bool state) {
    canvas->style.italic = state ? true : false;
}

void canvas_underline(struct canvas *canvas, bool state) {
    canvas->style.underline = state ? true : false;
}

void canvas_blink(struct canvas *canvas, bool state) {
    canvas->style.blink = state ? true : false;
}

void canvas_bold(struct canvas *canvas, bool state) {
    canvas->style.bold = state ? true : false;
}

void canvas_foreground(struct canvas *canvas, enum color color) {
    canvas->style.foreground = color;
}

void canvas_background(struct canvas *canvas, enum color color) {
    canvas->style.background = color;
}
