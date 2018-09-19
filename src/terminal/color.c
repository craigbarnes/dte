#include "color.h"
#include "../common.h"
#include "../completion.h"
#include "../editor.h"
#include "../error.h"
#include "../util/ptr-array.h"
#include "../util/strtonum.h"
#include "../util/xmalloc.h"

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
        editor.builtin_colors[i] = &find_color(builtin_color_names[i])->color;
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

#define CMP(str, val) cmp_str = str; cmp_val = val; goto compare

static unsigned short lookup_attr(const char *s, size_t len)
{
    const char *cmp_str;
    unsigned short cmp_val;
    switch (len) {
    case 3: CMP("dim", ATTR_DIM);
    case 5: CMP("blink", ATTR_BLINK);
    case 6: CMP("italic", ATTR_ITALIC);
    case 7: CMP("reverse", ATTR_REVERSE);
    case 12: CMP("lowintensity", ATTR_DIM);
    case 4:
        switch (s[0]) {
        case 'b': CMP("bold", ATTR_BOLD);
        case 'k': CMP("keep", ATTR_KEEP);
        }
        break;
    case 9:
        switch (s[0]) {
        case 'i': CMP("invisible", ATTR_INVIS);
        case 'u': CMP("underline", ATTR_UNDERLINE);
        }
        break;
    }
    return 0;
compare:
    return memcmp(s, cmp_str, len) ? 0 : cmp_val;
}

static short lookup_color(const char *s, size_t len)
{
    const char *cmp_str;
    short cmp_val;
    switch (len) {
    case 3: CMP("red", COLOR_RED);
    case 6: CMP("yellow", COLOR_YELLOW);
    case 8: CMP("darkgray", 8);
    case 4:
        switch (s[0]) {
        case 'b': CMP("blue", COLOR_BLUE);
        case 'c': CMP("cyan", COLOR_CYAN);
        case 'g': CMP("gray", COLOR_GRAY);
        case 'k': CMP("keep", -2);
        }
        break;
    case 5:
        switch (s[0]) {
        case 'b': CMP("black", COLOR_BLACK);
        case 'g': CMP("green", COLOR_GREEN);
        case 'w': CMP("white", 15);
        }
        break;
    case 7:
        switch (s[0]) {
        case 'd': CMP("default", COLOR_DEFAULT);
        case 'm': CMP("magenta", COLOR_MAGENTA);
        }
        break;
    }
    return -3;
compare:
    return memcmp(s, cmp_str, len) ? -3 : cmp_val;
}

static bool parse_color(const char *str, int *val)
{
    const char *ptr = str;
    long r, g, b;
    if (parse_long(&ptr, &r)) {
        if (*ptr == 0) {
            if (r < -2 || r > 255) {
                return false;
            }
            // color index -2..255
            *val = r;
            return true;
        }
        if (
            r < 0 || r > 5 || *ptr++ != '/' || !parse_long(&ptr, &g) ||
            g < 0 || g > 5 || *ptr++ != '/' || !parse_long(&ptr, &b) ||
            b < 0 || b > 5 || *ptr
        ) {
            return false;
        }

        // r/g/b to color index 16..231 (6x6x6 color cube)
        *val = 16 + r * 36 + g * 6 + b;
        return true;
    }

    size_t len = strlen(str);
    bool light = false;
    if (len >= 8 && memcmp(str, "light", 5) == 0) {
        light = true;
        str += 5;
        len -= 5;
    }

    const short c = lookup_color(str, len);
    switch (c) {
    case -3:
        return false;
    case COLOR_RED:
    case COLOR_GREEN:
    case COLOR_YELLOW:
    case COLOR_BLUE:
    case COLOR_MAGENTA:
    case COLOR_CYAN:
        *val = light ? c + 8 : c;
        return true;
    default:
        if (light) {
            return false;
        }
        *val = c;
        return true;
    }
}

static bool parse_attr(const char *str, unsigned short *attr)
{
    const unsigned short a = lookup_attr(str, strlen(str));
    if (a) {
        *attr |= a;
        return true;
    }

    return false;
}

bool parse_term_color(TermColor *color, char **strs)
{
    color->fg = -1;
    color->bg = -1;
    color->attr = 0;
    for (size_t i = 0, count = 0; strs[i]; i++) {
        const char *str = strs[i];
        int val;
        if (parse_color(str, &val)) {
            if (count > 1) {
                if (val == -2) {
                    // "keep" is also a valid attribute
                    color->attr |= ATTR_KEEP;
                } else {
                    error_msg("too many colors");
                    return false;
                }
            } else {
                if (!count) {
                    color->fg = val;
                } else {
                    color->bg = val;
                }
                count++;
            }
        } else if (!parse_attr(str, &color->attr)) {
            error_msg("invalid color or attribute %s", str);
            return false;
        }
    }
    return true;
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

void collect_colors_and_attributes(const char *prefix)
{
    static const char names[][16] = {
        "keep", "default", "black", "red", "green", "yellow",
        "blue", "magenta", "cyan", "gray", "darkgray", "lightred",
        "lightgreen", "lightyellow", "lightblue", "lightmagenta",
        "lightcyan", "white", "underline", "reverse", "blink",
        "dim", "bold", "invisible", "italic",
    };
    for (size_t i = 0; i < ARRAY_COUNT(names); i++) {
        if (str_has_prefix(names[i], prefix)) {
            add_completion(xstrdup(names[i]));
        }
    }
}
