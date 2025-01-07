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

typedef struct {
    char name[15];
    uint8_t len;
} ShortStr;

#define S(str) {str, STRLEN(str)}

static const ShortStr attr_names[] = {
    S("keep"),
    S("underline"),
    S("reverse"),
    S("blink"),
    S("dim"),
    S("bold"),
    S("invisible"),
    S("italic"),
    S("strikethrough"),
};

static const ShortStr color_names[] = {
    S("keep"),
    S("default"),
    S("black"),
    S("red"),
    S("green"),
    S("yellow"),
    S("blue"),
    S("magenta"),
    S("cyan"),
    S("gray"),
    S("darkgray"),
    S("lightred"),
    S("lightgreen"),
    S("lightyellow"),
    S("lightblue"),
    S("lightmagenta"),
    S("lightcyan"),
    S("white"),
};

UNITTEST {
    CHECK_STRUCT_ARRAY(attr_names, name);
    CHECK_STRUCT_ARRAY(color_names, name);
}

static unsigned int lookup_attr(const char *str)
{
    for (size_t i = 0; i < ARRAYLEN(attr_names); i++) {
        if (streq(str, attr_names[i].name)) {
            return 1U << i;
        }
    }
    if (streq(str, "lowintensity")) {
        return ATTR_DIM;
    }
    return 0;
}

static int32_t lookup_color(const char *str)
{
    return SSTR_TO_ENUM_WITH_OFFSET(str, color_names, name, COLOR_INVALID, -2);
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
ssize_t parse_term_style(TermStyle *style, char **strs, size_t nstrs)
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

    *style = (TermStyle) {
        .fg = colors[0],
        .bg = colors[1],
        .attr = attrs
    };
    return i;
}

void collect_colors_and_attributes(PointerArray *a, const char *prefix)
{
    size_t prefix_len = strlen(prefix);
    for (size_t i = 1; i < ARRAYLEN(color_names); i++) {
        const ShortStr *s = &color_names[i];
        if (str_has_strn_prefix(s->name, prefix, prefix_len)) {
            ptr_array_append(a, xmemdup(s->name, s->len + 1));
        }
    }
    for (size_t i = 0; i < ARRAYLEN(attr_names); i++) {
        const ShortStr *s = &attr_names[i];
        if (str_has_strn_prefix(s->name, prefix, prefix_len)) {
            ptr_array_append(a, xmemdup(s->name, s->len + 1));
        }
    }
}

size_t color_to_str(char buf[COLOR_STR_BUFSIZE], int32_t color)
{
    BUG_ON(!color_is_valid(color));
    if (color < 16) {
        const ShortStr *s = &color_names[color + 2];
        static_assert(sizeof(s->name) <= COLOR_STR_BUFSIZE);
        memcpy(buf, s->name, sizeof(s->name));
        return s->len;
    }

    if (color < 256) {
        return buf_u8_to_str((uint8_t)color, buf);
    }

    size_t i = 0;
    buf[i++] = '#';
    i += hex_encode_byte(buf + i, color_r(color));
    i += hex_encode_byte(buf + i, color_g(color));
    i += hex_encode_byte(buf + i, color_b(color));
    BUG_ON(i != 7);
    return i;
}

const char *term_style_to_string(char buf[TERM_STYLE_BUFSIZE], const TermStyle *style)
{
    size_t pos = color_to_str(buf, style->fg);

    if (style->bg != COLOR_DEFAULT || (style->attr & ATTR_KEEP) != 0) {
        buf[pos++] = ' ';
        pos += color_to_str(buf + pos, style->bg);
    }

    for (size_t i = 0; i < ARRAYLEN(attr_names); i++) {
        if (style->attr & (1U << i)) {
            const ShortStr *s = &attr_names[i];
            BUG_ON(pos + 2 + sizeof(s->name) >= TERM_STYLE_BUFSIZE);
            buf[pos++] = ' ';
            memcpy(buf + pos, s->name, sizeof(s->name));
            pos += s->len;
        }
    }

    buf[pos] = '\0';
    return buf;
}
