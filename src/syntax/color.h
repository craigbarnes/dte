#ifndef SYNTAX_COLOR_H
#define SYNTAX_COLOR_H

#include "../terminal/color.h"

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
    char *name;
    TermColor color;
} HlColor;

extern TermColor *builtin_colors[NR_BC];

void fill_builtin_colors(void);
HlColor *set_highlight_color(const char *name, const TermColor *color);
HlColor *find_color(const char *name);
void remove_extra_colors(void);
void collect_hl_colors(const char *prefix);

#endif
