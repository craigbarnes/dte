#ifndef SYNTAX_COLOR_H
#define SYNTAX_COLOR_H

#include "terminal/style.h"
#include "util/hashmap.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string.h"

typedef enum {
    BSE_ACTIVETAB,
    BSE_COMMANDLINE,
    BSE_CURRENTLINE,
    BSE_DEFAULT,
    BSE_DIALOG,
    BSE_ERRORMSG,
    BSE_INACTIVETAB,
    BSE_INFOMSG,
    BSE_LINENUMBER,
    BSE_NOLINE,
    BSE_NONTEXT,
    BSE_SELECTION,
    BSE_STATUSLINE,
    BSE_TABBAR,
    BSE_WSERROR,
    NR_BSE
} BuiltinStyleEnum;

typedef struct {
    TermStyle builtin[NR_BSE];
    HashMap other;
} ColorScheme;

void set_highlight_style(ColorScheme *colors, const char *name, const TermStyle *style) NONNULL_ARGS;
const TermStyle *find_style(const ColorScheme *colors, const char *name) NONNULL_ARGS;
void clear_hl_styles(ColorScheme *colors) NONNULL_ARGS;
void collect_builtin_styles(PointerArray *a, const char *prefix) NONNULL_ARGS;
void string_append_hl_style(String *s, const char *name, const TermStyle *style) NONNULL_ARGS;
String dump_hl_styles(const ColorScheme *colors) NONNULL_ARGS;

#endif
