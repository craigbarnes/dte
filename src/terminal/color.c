#include <string.h>
#include "color.h"
#include "completion.h"
#include "error.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/str-util.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"

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

static unsigned int lookup_attr(const char *s)
{
    for (size_t i = 0; i < ARRAY_COUNT(attr_names); i++) {
        if (streq(s, attr_names[i])) {
            return 1U << i;
        }
    }
    if (streq(s, "lowintensity")) {
        return ATTR_DIM;
    }
    return 0;
}

static int32_t lookup_color(const char *s)
{
    for (size_t i = 0; i < ARRAY_COUNT(color_names); i++) {
        if (streq(s, color_names[i])) {
            return i - 2;
        }
    }
    return COLOR_INVALID;
}

static int32_t parse_rrggbb(const char *str)
{
    int32_t color = 0;
    for (size_t i = 0; i < 6; i++) {
        unsigned int digit = hex_decode(str[i]);
        if (unlikely(digit > 0xF)) {
            return COLOR_INVALID;
        }
        color = (color << 4) | digit;
    }
    return COLOR_RGB(color);
}

UNITTEST {
    BUG_ON(parse_rrggbb("f01cff") != COLOR_RGB(0xf01cff));
    BUG_ON(parse_rrggbb("011011") != COLOR_RGB(0x011011));
    BUG_ON(parse_rrggbb("fffffg") != COLOR_INVALID);
    BUG_ON(parse_rrggbb(".") != COLOR_INVALID);
    BUG_ON(parse_rrggbb("") != COLOR_INVALID);
    BUG_ON(parse_rrggbb("11223") != COLOR_INVALID);
}

static int32_t parse_color(const char *str)
{
    size_t len = strlen(str);
    if (unlikely(len == 0)) {
        return COLOR_INVALID;
    }

    // Parse #rrggbb
    if (str[0] == '#') {
        if (unlikely(len != 7)) {
            return COLOR_INVALID;
        }
        return parse_rrggbb(str + 1);
    }

    // Parse r/g/b
    if (len == 5 && str[1] == '/') {
        uint8_t r = ((uint8_t)str[0]) - '0';
        uint8_t g = ((uint8_t)str[2]) - '0';
        uint8_t b = ((uint8_t)str[4]) - '0';
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

UNITTEST {
    BUG_ON(parse_color("-2") != COLOR_KEEP);
    BUG_ON(parse_color("-1") != COLOR_DEFAULT);
    BUG_ON(parse_color("0") != COLOR_BLACK);
    BUG_ON(parse_color("1") != COLOR_RED);
    BUG_ON(parse_color("255") != 255);
    BUG_ON(parse_color("0/0/0") != 16);
    BUG_ON(parse_color("2/3/4") != 110);
    BUG_ON(parse_color("5/5/5") != 231);
    BUG_ON(parse_color("black") != COLOR_BLACK);
    BUG_ON(parse_color("white") != COLOR_WHITE);
    BUG_ON(parse_color("keep") != COLOR_KEEP);
    BUG_ON(parse_color("default") != COLOR_DEFAULT);
    BUG_ON(parse_color("#abcdef") != COLOR_RGB(0xABCDEF));
    BUG_ON(parse_color("#a1b2c3") != COLOR_RGB(0xA1B2C3));
    BUG_ON(parse_color("-3") != COLOR_INVALID);
    BUG_ON(parse_color("256") != COLOR_INVALID);
    BUG_ON(parse_color("//0/0") != COLOR_INVALID);
    BUG_ON(parse_color("0/0/:") != COLOR_INVALID);
    BUG_ON(parse_color("lightblack") != COLOR_INVALID);
    BUG_ON(parse_color("lightwhite") != COLOR_INVALID);
    BUG_ON(parse_color("light_") != COLOR_INVALID);
    BUG_ON(parse_color("") != COLOR_INVALID);
    BUG_ON(parse_color(".") != COLOR_INVALID);
    BUG_ON(parse_color("#11223") != COLOR_INVALID);
    BUG_ON(parse_color("#fffffg") != COLOR_INVALID);
}

bool parse_term_color(TermColor *color, char **strs)
{
    color->fg = COLOR_DEFAULT;
    color->bg = COLOR_DEFAULT;
    color->attr = 0;
    for (size_t i = 0, count = 0; strs[i]; i++) {
        const char *const str = strs[i];
        const int32_t val = parse_color(str);
        if (val != COLOR_INVALID) {
            if (count > 1) {
                if (val == COLOR_KEEP) {
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
            continue;
        }
        const unsigned int attr = lookup_attr(str);
        if (attr) {
            color->attr |= attr;
            continue;
        }
        error_msg("invalid color or attribute %s", str);
        return false;
    }
    return true;
}

static int color_dist_sq (
    uint8_t R, uint8_t G, uint8_t B,
    uint8_t r, uint8_t g, uint8_t b
) {
    return (R - r) * (R - r) + (G - g) * (G - g) + (B - b) * (B - b);
}

// Convert RGB color component (0-255) to nearest xterm color cube index (0-5)
static uint8_t nearest_cube_index(uint8_t c)
{
    if (c < 75) {
        c += 27;
    }
    return (c - 35) / 40;
}

static uint8_t color_rgb_to_256(uint32_t color, bool *exact)
{
    static const uint8_t color_stops[6] = {
        0x00, 0x5f, 0x87,
        0xaf, 0xd7, 0xff
    };

    if ((color & COLOR_FLAG_RGB) == 0) {
        BUG_ON(color > 255);
        *exact = true;
        return color;
    }

    uint8_t r, g, b;
    color_split_rgb(color, &r, &g, &b);
    uint8_t r_idx = nearest_cube_index(r);
    uint8_t g_idx = nearest_cube_index(g);
    uint8_t b_idx = nearest_cube_index(b);
    uint8_t r_stop = color_stops[r_idx];
    uint8_t g_stop = color_stops[g_idx];
    uint8_t b_stop = color_stops[b_idx];

    if (r_stop == r && g_stop == g && b_stop == b) {
        *exact = true;
        return 16 + (36 * r_idx) + (6 * g_idx) + b_idx;
    }

    if (r == g && r == b) {
        uint8_t v = r - 8;
        if (v <= 230 && v % 10 == 0) {
            *exact = true;
            return (v / 10) + 232;
        }
    }

    // Calculate closest gray
    int gray_avg = (r + g + b) / 3;
    int gray_idx = (gray_avg > 238) ? 23 : ((gray_avg - 3) / 10);
    int gray = 8 + (10 * gray_idx);

    int rgb_distance = color_dist_sq(r_stop, g_stop, b_stop, r, g, b);
    int gray_distance = color_dist_sq(gray, gray, gray, r, g, b);
    if (gray_distance < rgb_distance) {
        // Gray is closest match
        *exact = false;
        return 232 + gray_idx;
    } else {
        // RGB cube color is closest match
        *exact = false;
        return 16 + (36 * r_idx) + (6 * g_idx) + b_idx;
    }
}

// Convert a 24-bit RGB color to an xterm palette color if one matches
// exactly, or otherwise return the original color unchanged. This is
// useful for reducing the size of SGR sequences sent to the terminal.
static int32_t color_rgb_optimize(int32_t color)
{
    bool exact;
    int32_t new_color = color_rgb_to_256(color, &exact);
    return exact ? new_color : color;
}

static uint8_t color_256_to_16(uint8_t color)
{
    enum {
        k = COLOR_BLACK,
        r = COLOR_RED,
        g = COLOR_GREEN,
        y = COLOR_YELLOW,
        b = COLOR_BLUE,
        m = COLOR_MAGENTA,
        c = COLOR_CYAN,
        a = COLOR_GRAY,
        A = COLOR_DARKGRAY,
        R = COLOR_LIGHTRED,
        G = COLOR_LIGHTGREEN,
        Y = COLOR_LIGHTYELLOW,
        B = COLOR_LIGHTBLUE,
        M = COLOR_LIGHTMAGENTA,
        C = COLOR_LIGHTCYAN,
        W = COLOR_WHITE
    };

    static const uint8_t table[256] = {
        k, r, g, y, b, m, c, a, A, R, G, Y, B, M, C, W, //   0...15
        k, b, b, b, B, B, g, c, b, b, B, B, g, g, c, b, //  16...31
        B, B, g, g, g, c, B, B, G, G, G, C, C, B, G, G, //  32...47
        G, G, C, C, r, m, m, m, m, B, y, A, b, b, B, B, //  48...63
        g, g, c, b, B, B, g, g, g, c, B, B, G, G, G, G, //  64...79
        C, B, G, G, G, G, G, C, r, m, m, m, m, m, y, r, //  80...95
        m, m, m, m, y, y, A, b, B, B, g, g, g, c, B, B, //  96..111
        G, G, G, G, C, B, G, G, G, G, G, C, r, r, m, m, // 112..127
        m, m, r, r, r, m, M, M, y, y, r, m, M, M, y, y, // 128..143
        y, a, B, B, G, G, G, G, C, B, G, G, G, G, G, C, // 144..159
        R, R, R, m, M, M, R, R, M, M, M, M, R, R, R, R, // 160..175
        M, M, y, y, y, M, M, M, Y, Y, Y, Y, a, B, Y, G, // 176..191
        G, G, G, C, R, R, R, M, M, M, R, R, R, R, R, M, // 192..207
        R, R, R, M, M, M, y, y, y, R, M, M, y, y, Y, Y, // 208..223
        R, M, Y, Y, Y, Y, Y, W, k, k, k, k, k, k, A, A, // 224..239
        A, A, A, A, a, a, a, a, a, a, W, W, W, W, W, W  // 240..255
    };

    return table[color];
}

static uint8_t color_any_to_256(int32_t color)
{
    BUG_ON(color < 0);
    bool exact;
    return color_rgb_to_256(color, &exact);
}

static uint8_t color_any_to_16(int32_t color)
{
    return color_256_to_16(color_any_to_256(color));
}

static uint8_t color_any_to_8(int32_t color)
{
    return color_any_to_16(color) & 7;
}

int32_t color_to_nearest(int32_t color, TermColorCapabilityType type)
{
    if (color < 0) {
        return color;
    }
    switch (type) {
    case TERM_0_COLOR: return COLOR_DEFAULT;
    case TERM_8_COLOR: return color_any_to_8(color);
    case TERM_16_COLOR: return color_any_to_16(color);
    case TERM_256_COLOR: return color_any_to_256(color);
    case TERM_TRUE_COLOR: return color_rgb_optimize(color);
    }
    BUG("unexpected TermColorCapabilityType value");
    // This should never be reached, but it silences compiler warnings
    // when DEBUG == 0 and __builtin_unreachable() isn't supported
    // (i.e. BUG() expands to nothing).
    return COLOR_DEFAULT;
}

void collect_colors_and_attributes(const char *prefix)
{
    for (size_t i = 1; i < ARRAY_COUNT(color_names); i++) {
        if (str_has_prefix(color_names[i], prefix)) {
            add_completion(xstrdup(color_names[i]));
        }
    }
    for (size_t i = 0; i < ARRAY_COUNT(attr_names); i++) {
        if (str_has_prefix(attr_names[i], prefix)) {
            add_completion(xstrdup(attr_names[i]));
        }
    }
}

static size_t append_color(char *buf, int32_t color)
{
    if (color < 16) {
        BUG_ON(color <= COLOR_INVALID);
        const char *name = color_names[color + 2];
        size_t len = strlen(name);
        memcpy(buf, name, len);
        return len;
    } else if (color < 256) {
        return xsnprintf(buf, 4, "%u", (unsigned int)color);
    }
    BUG_ON((color & COLOR_FLAG_RGB) == 0);
    uint8_t r, g, b;
    color_split_rgb(color, &r, &g, &b);
    return xsnprintf(buf, 8, "#%02hhx%02hhx%02hhx", r, g, b);
}

const char *term_color_to_string(const TermColor *color)
{
    static char buf[128];
    size_t pos = append_color(buf, color->fg);
    if (color->bg != COLOR_DEFAULT || (color->attr & ATTR_KEEP) != 0) {
        buf[pos++] = ' ';
        pos += append_color(buf + pos, color->bg);
    }
    for (size_t i = 0; i < ARRAY_COUNT(attr_names); i++) {
        if (color->attr & (1U << i)) {
            buf[pos++] = ' ';
            size_t len = strlen(attr_names[i]);
            memcpy(buf + pos, attr_names[i], len);
            pos += len;
        }
    }
    buf[pos] = '\0';
    return buf;
}
