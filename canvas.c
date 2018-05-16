#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "canvas.h"
#include "util.h"

void canvas_init(struct canvas *canvas, unsigned w, unsigned h) {
    canvas->canvas = calloc(w * h, sizeof(struct cell));
    canvas->w = w;
    canvas->h = h;

    canvas->window = false;
    canvas->window_canvas = NULL;
    canvas->window_x1 = canvas->window_y1 = canvas->window_x2 = canvas->window_y2 = 0;

    canvas_reset(canvas);

    canvas->flush_index = 0;
    CANVAS_CELL_CLEAR(canvas->flush_state);
    canvas->flush_encode_offset = 0;
    canvas->flush_last_index = 0;

    // For the sake of consistency, all blank cells store spaces
    // This also marks them all as dirty so that the first flush clears everything
    canvas_erase(canvas);
}

void canvas_free(struct canvas *canvas) {
    free(canvas->canvas);
}

void canvas_resize(struct canvas *canvas, unsigned w, unsigned h) {
    // Resize a window only
    if (canvas->window) {
        //TODO
    }
        // Resize an actual canvas
    else {
        if (canvas->w == w && canvas->h == h) {
            return;
        }

        // Copy the existing canvas and initialize a new one
        struct canvas old = *canvas;
        canvas_init(canvas, w, h);

        // Copy the existing canvas over and mark it as dirty
        unsigned x2 = (old.w > w) ? w : old.w, y2 = (old.h > h) ? h : old.h;
        for (unsigned y = 0; y < y2; y++) {
            for (unsigned x = 0; x < x2; x++) {
                struct cell old_cell = old.canvas[x + y * old.w];
                old_cell.dirty = true;
                canvas->canvas[x + y * w] = old_cell;
            }
        }

        // Free the existing canvas
        canvas_free(&old);
    }
}

void canvas_erase(struct canvas *canvas) {
    unsigned x1 = 0, y1 = 0, x2 = canvas->w, y2 = canvas->h;
    struct canvas *window = canvas;
    if (canvas->window) {
        canvas = canvas->window_canvas;

        x1 = window->window_x1, y1 = window->window_y1;
        x2 = window->window_x2, y2 = window->window_y2;
    }

    struct cell new_cell = canvas->style;
    new_cell.code_point = ' ';
    new_cell.dirty = true;

    for (unsigned y = y1; y < y2; y++) {
        for (unsigned x = x1; x < x2; x++) {
            struct cell *cell = &canvas->canvas[x + y * canvas->w];
            if (!CANVAS_CELL_EQ(new_cell, *cell)) {
                *cell = new_cell;
            }
        }
    }
}

#define CANVAS_LEN (canvas->w * canvas->h)

bool canvas_flush(struct canvas *canvas, char *buf, size_t len, size_t *len_written) {
    assert(!canvas->window);

    if (canvas->flush_index == CANVAS_LEN) {
        return false;
    }

    size_t index = canvas->flush_index, remaining = len;
    while (remaining > 0 && index < CANVAS_LEN) {
        struct cell *cell = &canvas->canvas[index];
        // Ignore unchanged cells at the cost of a cursor move which may
        // potentially be longer
        if (!cell->dirty) {
            index++;
            continue;
        }

        if (canvas->flush_last_index + 1 != index) {
            int x = (int) (index % canvas->w), y = (int) (index / canvas->w);
            int escape_len = snprintf(buf, remaining, "\x1b[%d;%dH", y + 1, x + 1);

            buf += escape_len;
            remaining -= escape_len;
        }

        canvas->flush_last_index = index;

        //TODO parse state and emit escape sequences

        size_t encoded_len = utf8_encode(canvas->flush_encode_offset, &buf, remaining,
                                         cell->code_point);

        // The encoded character didn't entirely fit, so we'll need to send the
        // rest next flush
        if (encoded_len > remaining) {
            canvas->flush_encode_offset = encoded_len - remaining;
            remaining = 0;
            break;
        }

        cell->dirty = false;

        canvas->flush_encode_offset = 0;

        remaining -= encoded_len;
        index++;
    }

    canvas->flush_index = index;
    *len_written = len - remaining;
    return true;
}

bool canvas_forced_flush(struct canvas *canvas, char *buf, size_t len, size_t *len_written) {
    assert(!canvas->window);

    // Mark all bits as dirty
    for (unsigned y = 0; y < canvas->h; y++) {
        for (unsigned x = 0; x < canvas->w; x++) {
            canvas->canvas[x + y * canvas->w].dirty = true;
        }
    }

    return canvas_flush(canvas, buf, len, len_written);
}

void canvas_write_utf8(struct canvas *canvas, unsigned x, unsigned y, char *msg) {
    struct canvas *window = canvas;
    if (canvas->window) {
        canvas = canvas->window_canvas;
    }

    canvas->flush_index = 0;

    //TODO decode utf8
}

void canvas_write_all_utf32(struct canvas *canvas, unsigned long *buf, size_t len) {
    unsigned x1 = 0, y1 = 0, x2 = canvas->w, y2 = canvas->h;

    struct canvas *window = canvas;
    if (canvas->window) {
        canvas = canvas->window_canvas;

        x1 = window->window_x1, y1 = window->window_y1;
        x2 = window->window_x2, y2 = window->window_y2;
    }

    for (unsigned y = y1; y < y2; y++) {
        struct cell *cell = &canvas->canvas[y * canvas->w];
        for (unsigned x = x1; x < x2; x++) {
            if (cell->code_point != *buf) {
                cell->code_point = *buf;
                cell->dirty = true;
            }

            buf++, cell++;
        }
    }
}

void canvas_put(struct canvas *canvas, unsigned x, unsigned y, unsigned long c) {
    struct canvas *window = canvas;
    if (canvas->window) {
        canvas = canvas->window_canvas;

        x += window->window_x1;
        y += window->window_y1;
    }

    struct cell *cell = &canvas->canvas[x + y * canvas->w];
    if (cell->code_point != c) {
        cell->code_point = c;
        cell->dirty = true;

        canvas->flush_index = 0;
    }
}

unsigned long canvas_get(struct canvas *canvas, unsigned x, unsigned y) {
    struct canvas *window = canvas;
    if (canvas->window) {
        canvas = canvas->window_canvas;

        x += window->window_x1;
        y += window->window_y1;
    }

    return canvas->canvas[x + y * canvas->w].code_point;
}

void canvas_create_window(struct canvas *canvas, unsigned x1, unsigned y1,
                          unsigned x2, unsigned y2, struct canvas *window) {
    // Don't create a window of a window, but rather a different window of the
    // original canvas
    if (canvas->window) {
        canvas = canvas->window_canvas;
    }

    window->canvas = NULL;
    window->w = x2 - x1;
    window->h = y2 - y1;

    window->flush_index = 0;

    window->window = true;
    window->window_canvas = canvas;
    window->window_x1 = x1;
    window->window_y1 = y1;
    window->window_x2 = x2;
    window->window_y2 = y2;
}

void canvas_rect(struct canvas *canvas, unsigned x, unsigned y, unsigned w, unsigned h,
                 unsigned long symbol) {
    struct canvas *window = canvas;
    if (canvas->window) {
        canvas = canvas->window_canvas;

        x += window->window_x1;
        y += window->window_y1;
    }

    assert(x + w <= canvas->w);
    assert(y + h <= canvas->h);

    struct cell new_cell = canvas->style;
    new_cell.code_point = symbol;
    new_cell.dirty = true;

    for (unsigned row = y; row < y + h; row++) {
        int step = (row == y || row == y + h - 1) ? 1 : w - 1;
        for (unsigned col = x; col < x + w; col += step) {
            struct cell *cell = &(canvas->canvas[col + row * canvas->w]);
            if (!CANVAS_CELL_EQ(new_cell, *cell)) {
                *cell = new_cell;
            }
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
