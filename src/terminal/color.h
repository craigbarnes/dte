#ifndef TERMINAL_COLOR_H
#define TERMINAL_COLOR_H

#include <stdbool.h>
#include <stdint.h>
#include "../util/macros.h"

#define COLOR_FLAG_RGB INT32_C(0x01000000)

typedef enum {
    TERM_0_COLOR,
    TERM_8_COLOR,
    TERM_16_COLOR,
    TERM_256_COLOR,
    TERM_TRUE_COLOR
} TermColorCapabilityType;

enum {
    COLOR_INVALID = -3,
    COLOR_KEEP = -2,
    COLOR_DEFAULT = -1,
    COLOR_BLACK = 0,
    COLOR_RED = 1,
    COLOR_GREEN = 2,
    COLOR_YELLOW = 3,
    COLOR_BLUE = 4,
    COLOR_MAGENTA = 5,
    COLOR_CYAN = 6,
    COLOR_GRAY = 7
};

enum {
    ATTR_KEEP = 0x01,
    ATTR_UNDERLINE = 0x02,
    ATTR_REVERSE = 0x04,
    ATTR_BLINK = 0x08,
    ATTR_DIM = 0x10,
    ATTR_BOLD = 0x20,
    ATTR_INVIS = 0x40,
    ATTR_ITALIC = 0x80,
    ATTR_STRIKETHROUGH = 0x100,
};

typedef struct {
    int32_t fg;
    int32_t bg;
    unsigned short attr;
} TermColor;

static inline void color_split_rgb(int32_t c, uint8_t *r, uint8_t *g, uint8_t *b)
{
    *r = (c >> 16) & 0xff;
    *g = (c >> 8) & 0xff;
    *b = c & 0xff;
}

static inline PURE bool same_color(const TermColor *c1, const TermColor *c2)
{
    return
        c1->attr == c2->attr
        && c1->fg == c2->fg
        && c1->bg == c2->bg
    ;
}

bool parse_term_color(TermColor *color, char **strs);
int32_t convert_color_to_nearest_supported(int32_t color);

#endif
