#ifndef COLOR_H
#define COLOR_H

#include <stdbool.h>
#include "term.h"

typedef struct {
    char *name;
    TermColor color;
} HlColor;

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

void fill_builtin_colors(void);
HlColor *set_highlight_color(const char *name, const TermColor *color);
HlColor *find_color(const char *name);
void remove_extra_colors(void);
bool parse_term_color(TermColor *color, char **strs);
void collect_hl_colors(const char *prefix);
void collect_colors_and_attributes(const char *prefix);

#endif
