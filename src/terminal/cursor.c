#include <string.h>
#include "cursor.h"
#include "util/array.h"
#include "util/numtostr.h"
#include "util/debug.h"

static const char cursor_modes[NR_CURSOR_MODES][12] = {
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

UNITTEST {
    CHECK_STRING_ARRAY(cursor_modes);
    CHECK_STRING_ARRAY(cursor_types);
    CHECK_STRING_ARRAY(cursor_colors);
}

const char *cursor_mode_to_str(CursorInputMode mode)
{
    BUG_ON(mode < 0 || mode >= ARRAYLEN(cursor_modes));
    return cursor_modes[mode];
}

const char *cursor_type_to_str(TermCursorType type)
{
    BUG_ON(type < 0 || type >= ARRAYLEN(cursor_types));
    return cursor_types[type];
}

const char *cursor_color_to_str(int32_t c)
{
    BUG_ON(!(c == COLOR_KEEP || c == COLOR_DEFAULT || c & COLOR_FLAG_RGB));
    static char buf[8];
    size_t n = color_to_str(buf, c);
    BUG_ON(n < 4 || n >= sizeof(buf));
    buf[n] = '\0';
    return buf;
}

CursorInputMode cursor_mode_from_str(const char *name)
{
    return STR_TO_ENUM(name, cursor_modes, NR_CURSOR_MODES);
}

TermCursorType cursor_type_from_str(const char *name)
{
    return STR_TO_ENUM(name, cursor_types, CURSOR_INVALID);
}

int32_t cursor_color_from_str(const char *str)
{
    if (str[0] == '#') {
        return parse_rgb(str + 1, strlen(str + 1));
    }
    return STR_TO_ENUM_WITH_OFFSET(str, cursor_colors, COLOR_INVALID, -2);
}

void collect_cursor_modes(PointerArray *a, const char *prefix)
{
    COLLECT_STRINGS(cursor_modes, a, prefix);
}

void collect_cursor_types(PointerArray *a, const char *prefix)
{
    COLLECT_STRINGS(cursor_types, a, prefix);
}

void collect_cursor_colors(PointerArray *a, const char *prefix)
{
    COLLECT_STRINGS(cursor_colors, a, prefix);
}
