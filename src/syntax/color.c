#include <string.h>
#include "color.h"
#include "../debug.h"
#include "../completion.h"
#include "../util/macros.h"
#include "../util/ptr-array.h"
#include "../util/str-util.h"
#include "../util/xmalloc.h"

TermColor *builtin_colors[NR_BC];

static PointerArray hl_colors = PTR_ARRAY_INIT;

static const char builtin_color_names[NR_BC][16] = {
    [BC_DEFAULT] = "default",
    [BC_NONTEXT] = "nontext",
    [BC_NOLINE] = "noline",
    [BC_WSERROR] = "wserror",
    [BC_SELECTION] = "selection",
    [BC_CURRENTLINE] = "currentline",
    [BC_LINENUMBER] = "linenumber",
    [BC_STATUSLINE] = "statusline",
    [BC_COMMANDLINE] = "commandline",
    [BC_ERRORMSG] = "errormsg",
    [BC_INFOMSG] = "infomsg",
    [BC_TABBAR] = "tabbar",
    [BC_ACTIVETAB] = "activetab",
    [BC_INACTIVETAB] = "inactivetab",
};

UNITTEST {
    for (size_t i = 0; i < ARRAY_COUNT(builtin_color_names); i++) {
        const char *const name = builtin_color_names[i];
        if (name[0] == '\0') {
            BUG("missing string at builtin_color_names[%zu]", i);
        }
        if (memchr(name, '\0', sizeof(builtin_color_names[0])) == NULL) {
            BUG("builtin_color_names[%zu] missing null-terminator", i);
        }
    }
}

void fill_builtin_colors(void)
{
    for (size_t i = 0; i < NR_BC; i++) {
        builtin_colors[i] = &find_color(builtin_color_names[i])->color;
    }
}

HlColor *set_highlight_color(const char *name, const TermColor *color)
{
    for (size_t i = 0, n = hl_colors.count; i < n; i++) {
        HlColor *c = hl_colors.ptrs[i];
        if (streq(name, c->name)) {
            c->color = *color;
            return c;
        }
    }

    HlColor *c = xnew(HlColor, 1);
    c->name = xstrdup(name);
    c->color = *color;
    ptr_array_add(&hl_colors, c);
    return c;
}

static HlColor *find_real_color(const char *name)
{
    for (size_t i = 0; i < hl_colors.count; i++) {
        HlColor *c = hl_colors.ptrs[i];
        if (streq(c->name, name)) {
            return c;
        }
    }
    return NULL;
}

HlColor *find_color(const char *name)
{
    HlColor *color = find_real_color(name);
    if (color) {
        return color;
    }

    const char *dot = strchr(name, '.');
    if (dot) {
        return find_real_color(dot + 1);
    }

    return NULL;
}

// NOTE: you have to call update_all_syntax_colors() after this
void remove_extra_colors(void)
{
    BUG_ON(hl_colors.count < NR_BC);
    for (size_t i = NR_BC; i < hl_colors.count; i++) {
        HlColor *c = hl_colors.ptrs[i];

        // Make possible use after free error easy to see
        c->color.fg = COLOR_RED;
        c->color.bg = COLOR_YELLOW;
        c->color.attr = ATTR_BOLD;
        free(c->name);
        c->name = NULL;
        free(c);

        hl_colors.ptrs[i] = NULL;
    }
    hl_colors.count = NR_BC;
}

void collect_hl_colors(const char *prefix)
{
    for (size_t i = 0, n = hl_colors.count; i < n; i++) {
        const HlColor *c = hl_colors.ptrs[i];
        if (str_has_prefix(c->name, prefix)) {
            add_completion(xstrdup(c->name));
        }
    }
}
