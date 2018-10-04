#ifndef TERMINAL_COLOR_H
#define TERMINAL_COLOR_H

#include <stdbool.h>

enum {
    COLOR_DEFAULT = -1,
    COLOR_BLACK,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_GRAY
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
};

typedef struct {
    short fg;
    short bg;
    unsigned short attr;
} TermColor;

bool parse_term_color(TermColor *color, char **strs);

static inline bool same_color(const TermColor *c1, const TermColor *c2)
{
    return
        c1->attr == c2->attr
        && c1->fg == c2->fg
        && c1->bg == c2->bg
    ;
}

#endif
