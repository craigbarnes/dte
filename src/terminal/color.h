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

enum builtin_color {
    BC_DEFAULT,
    BC_NONTEXT,
    BC_NOLINE,
    BC_WSERROR,
    BC_SELECTION,
    BC_CURRENTLINE,
    BC_LINENUMBER,
    BC_STATUSLINE,
    BC_COMMANDLINE,
    BC_ERRORMSG,
    BC_INFOMSG,
    BC_TABBAR,
    BC_ACTIVETAB,
    BC_INACTIVETAB,
    NR_BC
};

typedef struct {
    short fg;
    short bg;
    unsigned short attr;
} TermColor;

typedef struct {
    char *name;
    TermColor color;
} HlColor;

void fill_builtin_colors(void);
HlColor *set_highlight_color(const char *name, const TermColor *color);
HlColor *find_color(const char *name);
void remove_extra_colors(void);
bool parse_term_color(TermColor *color, char **strs);
void collect_hl_colors(const char *prefix);
void collect_colors_and_attributes(const char *prefix);

static inline bool same_color(const TermColor *c1, const TermColor *c2)
{
    return
        c1->attr == c2->attr
        && c1->fg == c2->fg
        && c1->bg == c2->bg
    ;
}

#endif
