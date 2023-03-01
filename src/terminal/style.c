#include <limits.h>
#include <string.h>
#include "style.h"
#include "color.h"
#include "util/array.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/numtostr.h"
#include "util/str-util.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"

static const char attr_names[][16] = {
    "keep",
    "underline",
    "reverse",
    "blink",
    "dim",
    "bold",
    "invisible",
    "italic",
    "strikethrough"
};

static const char color_names[][16] = {
    "keep",
    "default",
    "black",
    "red",
    "green",
    "yellow",
    "blue",
    "magenta",
    "cyan",
    "gray",
    "darkgray",
    "lightred",
    "lightgreen",
    "lightyellow",
    "lightblue",
    "lightmagenta",
    "lightcyan",
    "white"
};

UNITTEST {
    CHECK_STRING_ARRAY(attr_names);
    CHECK_STRING_ARRAY(color_names);
}

static unsigned int lookup_attr(const char *s)
{
    for (size_t i = 0; i < ARRAYLEN(attr_names); i++) {
        if (streq(s, attr_names[i])) {
            return 1U << i;
        }
    }
    if (streq(s, "lowintensity")) {
        return ATTR_DIM;
    }
    return 0;
}

static int32_t lookup_color(const char *name)
{
    return STR_TO_ENUM_WITH_OFFSET(name, color_names, COLOR_INVALID, -2);
}

static int32_t parse_color(const char *str)
{
    size_t len = strlen(str);
    if (unlikely(len == 0)) {
        return COLOR_INVALID;
    }

    // Parse #rgb or #rrggbb
    if (str[0] == '#') {
        return parse_rgb(str + 1, len - 1);
    }

    // Parse r/g/b
    if (len == 5 && str[1] == '/') {
        const unsigned char *u_str = str;
        uint8_t r = u_str[0] - '0';
        uint8_t g = u_str[2] - '0';
        uint8_t b = u_str[4] - '0';
        if (unlikely(r > 5 || g > 5 || b > 5 || str[3] != '/')) {
            return COLOR_INVALID;
        }
        // Convert to color index 16..231 (xterm 6x6x6 color cube)
        return 16 + (r * 36) + (g * 6) + b;
    }

    // Parse -2 .. 255
    if (len <= 3 && (str[0] == '-' || ascii_isdigit(str[0]))) {
        int x;
        if (unlikely(!str_to_int(str, &x) || x < -2 || x > 255)) {
            return COLOR_INVALID;
        }
        return x;
    }

    return lookup_color(str);
}

// Note: this function returns the number of valid strings parsed, or -1 if
// more than 2 valid colors were encountered. Thus, success is indicated by
// a return value equal to `nstrs`.
ssize_t parse_term_color(TermColor *color, char **strs, size_t nstrs)
{
    int32_t colors[2] = {COLOR_DEFAULT, COLOR_DEFAULT};
    unsigned int attrs = 0;
    size_t i = 0;

    for (size_t nr_colors = 0; i < nstrs; i++) {
        const char *str = strs[i];
        int32_t c = parse_color(str);
        if (c == COLOR_INVALID) {
            unsigned int attr = lookup_attr(str);
            if (likely(attr)) {
                attrs |= attr;
                continue;
            }
            // Invalid color or attribute
            return i;
        }
        if (nr_colors == ARRAYLEN(colors)) {
            if (likely(c == COLOR_KEEP)) {
                // "keep" is also a valid attribute
                attrs |= ATTR_KEEP;
                continue;
            }
            // Too many colors
            return -1;
        }
        colors[nr_colors++] = c;
    }

    *color = (TermColor) {
        .fg = colors[0],
        .bg = colors[1],
        .attr = attrs
    };
    return i;
}

void collect_colors_and_attributes(PointerArray *a, const char *prefix)
{
    for (size_t i = 1; i < ARRAYLEN(color_names); i++) {
        if (str_has_prefix(color_names[i], prefix)) {
            ptr_array_append(a, xstrdup(color_names[i]));
        }
    }
    for (size_t i = 0; i < ARRAYLEN(attr_names); i++) {
        if (str_has_prefix(attr_names[i], prefix)) {
            ptr_array_append(a, xstrdup(attr_names[i]));
        }
    }
}

size_t color_to_str(char *buf, int32_t color)
{
    if (color < 16) {
        BUG_ON(color <= COLOR_INVALID);
        const char *name = color_names[color + 2];
        size_t len = strlen(name);
        memcpy(buf, name, len);
        return len;
    } else if (color < 256) {
        return buf_uint_to_str((unsigned int)color, buf);
    }

    BUG_ON(!(color & COLOR_FLAG_RGB));
    buf[0] = '#';
    hex_encode_u24_fixed(buf + 1, color);
    return 7;
}

const char *term_color_to_string(const TermColor *color)
{
    static char buf[128];
    size_t pos = color_to_str(buf, color->fg);
    if (color->bg != COLOR_DEFAULT || (color->attr & ATTR_KEEP) != 0) {
        buf[pos++] = ' ';
        pos += color_to_str(buf + pos, color->bg);
    }
    for (size_t i = 0; i < ARRAYLEN(attr_names); i++) {
        if (color->attr & (1U << i)) {
            size_t len = strlen(attr_names[i]);
            BUG_ON(pos + len + 2 >= sizeof(buf));
            buf[pos++] = ' ';
            memcpy(buf + pos, attr_names[i], len);
            pos += len;
        }
    }
    buf[pos] = '\0';
    return buf;
}
