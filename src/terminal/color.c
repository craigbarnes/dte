#include <inttypes.h>
#include <string.h>
#include "color.h"
#include "../debug.h"
#include "../error.h"
#include "../util/ascii.h"
#include "../util/strtonum.h"

#define CMP(str, val) cmp_str = str; cmp_val = val; goto compare

static unsigned int lookup_attr(const char *s, size_t len)
{
    const char *cmp_str;
    unsigned int cmp_val;
    switch (len) {
    case 3: CMP("dim", ATTR_DIM);
    case 5: CMP("blink", ATTR_BLINK);
    case 6: CMP("italic", ATTR_ITALIC);
    case 7: CMP("reverse", ATTR_REVERSE);
    case 12: CMP("lowintensity", ATTR_DIM);
    case 13: CMP("strikethrough", ATTR_STRIKETHROUGH);
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

static int32_t lookup_color(const char *s, size_t len)
{
    const char *cmp_str;
    int32_t cmp_val;
    switch (len) {
    case 3: CMP("red", COLOR_RED);
    case 6: CMP("yellow", COLOR_YELLOW);
    case 8: CMP("darkgray", 8);
    case 4:
        switch (s[0]) {
        case 'b': CMP("blue", COLOR_BLUE);
        case 'c': CMP("cyan", COLOR_CYAN);
        case 'g': CMP("gray", COLOR_GRAY);
        case 'k': CMP("keep", COLOR_KEEP);
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
    return COLOR_INVALID;
compare:
    return memcmp(s, cmp_str, len) ? COLOR_INVALID : cmp_val;
}

static int32_t parse_rrggbb(const char *str)
{
    uint8_t digits[6];
    for (size_t i = 0; i < 6; i++) {
        int val = hex_decode(str[i]);
        if (val < 0) {
            return COLOR_INVALID;
        }
        digits[i] = val;
    }
    int32_t r = (digits[0] << 4) | digits[1];
    int32_t g = (digits[2] << 4) | digits[3];
    int32_t b = (digits[4] << 4) | digits[5];
    return r << 16 | g << 8 | b | COLOR_FLAG_RGB;
}

UNITTEST {
    BUG_ON(parse_rrggbb("f01cff") != COLOR_RGB(0xf01cff));
    BUG_ON(parse_rrggbb("011011") != COLOR_RGB(0x011011));
    BUG_ON(parse_rrggbb("fffffg") != COLOR_INVALID);
    BUG_ON(parse_rrggbb(".") != COLOR_INVALID);
    BUG_ON(parse_rrggbb("11223") != COLOR_INVALID);
}

static int32_t parse_color(const char *str)
{
    size_t len = strlen(str);
    if (len == 0) {
        return COLOR_INVALID;
    }

    // Parse #rrggbb
    if (str[0] == '#') {
        if (len != 7) {
            return COLOR_INVALID;
        }
        return parse_rrggbb(str + 1);
    }

    // Parse r/g/b
    if (len == 5 && str[1] == '/') {
        uint8_t r = ((uint8_t)str[0]) - '0';
        uint8_t g = ((uint8_t)str[2]) - '0';
        uint8_t b = ((uint8_t)str[4]) - '0';
        if (r > 5 || g > 5 || b > 5 || str[3] != '/') {
            return COLOR_INVALID;
        }
        // Convert to color index 16..231 (xterm 6x6x6 color cube)
        return 16 + r * 36 + g * 6 + b;
    }

    // Parse -2 .. 255
    if (len <= 3 && (str[0] == '-' || ascii_isdigit(str[0]))) {
        int x;
        if (!str_to_int(str, &x) || x < -2 || x > 255) {
            return COLOR_INVALID;
        }
        return x;
    }

    bool light = false;
    if (len >= 8 && memcmp(str, "light", 5) == 0) {
        light = true;
        str += 5;
        len -= 5;
    }

    const int32_t c = lookup_color(str, len);
    switch (c) {
    case COLOR_INVALID:
        return COLOR_INVALID;
    case COLOR_RED:
    case COLOR_GREEN:
    case COLOR_YELLOW:
    case COLOR_BLUE:
    case COLOR_MAGENTA:
    case COLOR_CYAN:
        return light ? c + 8 : c;
    default:
        return light ? COLOR_INVALID : c;
    }
}

static bool parse_attr(const char *str, unsigned int *attr)
{
    const unsigned int a = lookup_attr(str, strlen(str));
    if (a) {
        *attr |= a;
        return true;
    }

    return false;
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
        } else if (!parse_attr(str, &color->attr)) {
            error_msg("invalid color or attribute %s", str);
            return false;
        }
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
    if (c < 48) {
        return 0;
    }
    if (c < 114) {
        return 1;
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

    if (r >= 8 && r <= 238 && r == g && r == b) {
        uint8_t v = r - 8;
        if (v % 10 == 0) {
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
