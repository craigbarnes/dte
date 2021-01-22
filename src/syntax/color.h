#ifndef SYNTAX_COLOR_H
#define SYNTAX_COLOR_H

#include "terminal/color.h"
#include "util/string.h"

typedef enum {
    BC_ACTIVETAB,
    BC_COMMANDLINE,
    BC_CURRENTLINE,
    BC_DEFAULT,
    BC_DIALOG,
    BC_ERRORMSG,
    BC_INACTIVETAB,
    BC_INFOMSG,
    BC_LINENUMBER,
    BC_NOLINE,
    BC_NONTEXT,
    BC_SELECTION,
    BC_STATUSLINE,
    BC_TABBAR,
    BC_WSERROR,
    NR_BC
} BuiltinColorEnum;

extern TermColor builtin_colors[NR_BC];

void set_highlight_color(const char *name, const TermColor *color);
TermColor *find_color(const char *name);
void clear_hl_colors(void);
void collect_hl_colors(const char *prefix);
String dump_hl_colors(void);

#endif
