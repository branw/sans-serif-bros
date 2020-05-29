#ifndef SSB_CANVAS_H
#define SSB_CANVAS_H

#include <stdbool.h>
#include <stdint.h>

// Color values mapping to the standard 8 SGR (Select Graphics Rendition)
// attributes
enum color {
    black = 0,
    red = 1,
    green = 2,
    yellow = 3,
    blue = 4,
    magenta = 5,
    cyan = 6,
    white = 7
};

struct cell {
    uint32_t code_point : 21;
    uint32_t italic : 1;
    uint32_t underline : 1;
    uint32_t blink : 1;
    enum color foreground : 3;
    enum color background : 3;
    uint32_t bold : 1;
};

_Static_assert(sizeof(struct cell) == sizeof(uint32_t),
        "Canvas cell should fit in 32 bits");

// Zero out a cell
#define CANVAS_CELL_CLEAR(cell) do { (cell).code_point = 0; (cell).italic = false; \
        (cell).underline = false; (cell).blink = false; (cell).foreground = black; \
        (cell).background = black; (cell).bold = false; } while (0)

#define CANVAS_CELL_STYLE_EQ(a, b) ((a).italic == (b).italic && \
    (a).underline == (b).underline && (a).blink == (b).blink && (a).bold == (b).bold && \
    (a).foreground == (b).foreground && (a).background == (b).background && (a).bold == (b).bold)

// Check if two cells are equivalent
#define CANVAS_CELL_EQ(a, b) ((a).code_point == (b).code_point && CANVAS_CELL_STYLE_EQ((a), (b)))

struct canvas {
    struct cell *buf[2];
    unsigned w, h;

    struct cell style;

    bool force_flush;
    bool force_next_flush_only;
    size_t flush_index;
    struct cell flush_state;
};

void canvas_create(struct canvas *canvas, unsigned w, unsigned h);

void canvas_destroy(struct canvas *canvas);

void canvas_resize(struct canvas *canvas, unsigned w, unsigned h);

void canvas_erase(struct canvas *canvas);

bool canvas_flush(struct canvas *canvas, char *buf, size_t len, size_t *len_written);

void canvas_force_next_flush(struct canvas *canvas);

void canvas_write(struct canvas *canvas, unsigned x, unsigned y, char *msg);

void canvas_write_block(struct canvas *canvas, unsigned x1, unsigned y1, unsigned w,
                        unsigned h, char *buf);

void canvas_write_block_utf32(struct canvas *canvas, unsigned x1, unsigned y1, unsigned w,
                              unsigned h, uint32_t *buf, size_t len);

void canvas_put(struct canvas *canvas, unsigned x, unsigned y, unsigned long c);

void canvas_rect(struct canvas *canvas, unsigned x, unsigned y, unsigned w, unsigned h,
                 unsigned long symbol);

void canvas_reset(struct canvas *canvas);

void canvas_italic(struct canvas *canvas, bool state);

void canvas_underline(struct canvas *canvas, bool state);

void canvas_blink(struct canvas *canvas, bool state);

void canvas_bold(struct canvas *canvas, bool state);

void canvas_foreground(struct canvas *canvas, enum color color);

void canvas_background(struct canvas *canvas, enum color color);

#define CANVAS_PUT(x, y, sym) do { canvas_put(CANVAS, (x), (y), (sym)); } while (0)

#define CANVAS_ITALIC(x) do { \
        canvas_italic(CANVAS, true); do { x } while (0); canvas_italic(CANVAS, false); \
    } while(0)


#endif //SSB_CANVAS_H
