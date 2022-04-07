#include <stddef.h>
#include <string.h>
#include "cursor.h"
#include "util/str-util.h"

static const char modes[NR_CURSOR_MODES][12] = {
    [CURSOR_MODE_DEFAULT] = "default",
    [CURSOR_MODE_INSERT] = "insert",
    [CURSOR_MODE_OVERWRITE] = "overwrite",
    [CURSOR_MODE_CMDLINE] = "cmdline",
};

static const char cursor_types[][20] = {
    [CURSOR_DEFAULT] = "default",
    [CURSOR_BLINKING_BLOCK] = "blinking-block",
    [CURSOR_STEADY_BLOCK] = "block",
    [CURSOR_BLINKING_UNDERLINE] = "blinking-underline",
    [CURSOR_STEADY_UNDERLINE] = "underline",
    [CURSOR_BLINKING_BAR] = "blinking-bar",
    [CURSOR_STEADY_BAR] = "bar",
    [CURSOR_KEEP] = "keep",
};

static const char cursor_colors[][8] = {
    "keep",
    "default",
};

CursorInputMode cursor_mode_from_str(const char *name)
{
    for (CursorInputMode m = 0; m < ARRAYLEN(modes); m++) {
        if (streq(name, modes[m])) {
            return m;
        }
    }
    return NR_CURSOR_MODES; // Invalid name
}

TermCursorType cursor_type_from_str(const char *name)
{
    for (TermCursorType t = 0; t < ARRAYLEN(cursor_types); t++) {
        if (streq(name, cursor_types[t])) {
            return t;
        }
    }
    return CURSOR_INVALID;
}

int32_t cursor_color_from_str(const char *str)
{
    size_t len = strlen(str);
    if (unlikely(len == 0)) {
        return COLOR_INVALID;
    }

    if (str[0] == '#') {
        return parse_rgb(str + 1, len - 1);
    }

    static_assert(COLOR_KEEP == -2);
    static_assert(COLOR_DEFAULT == -1);
    for (int32_t i = 0; i < ARRAYLEN(cursor_colors); i++) {
        if (streq(str, cursor_colors[i])) {
            return i - 2;
        }
    }

    return COLOR_INVALID;
}

void collect_cursor_modes(PointerArray *a, const char *prefix)
{
    for (size_t i = 0; i < ARRAYLEN(modes); i++) {
        const char *str = modes[i];
        if (str_has_prefix(str, prefix)) {
            ptr_array_append(a, xstrdup(str));
        }
    }
}

void collect_cursor_types(PointerArray *a, const char *prefix)
{
    for (size_t i = 0; i < ARRAYLEN(cursor_types); i++) {
        const char *str = cursor_types[i];
        if (str_has_prefix(str, prefix)) {
            ptr_array_append(a, xstrdup(str));
        }
    }
}

void collect_cursor_colors(PointerArray *a, const char *prefix)
{
    for (size_t i = 0; i < ARRAYLEN(cursor_colors); i++) {
        const char *str = cursor_colors[i];
        if (str_has_prefix(str, prefix)) {
            ptr_array_append(a, xstrdup(str));
        }
    }
}
