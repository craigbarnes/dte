#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "color.h"
#include "terminal.h"
#include "../debug.h"
#include "../error.h"
#include "../util/ascii.h"
#include "../util/strtonum.h"

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

static int32_t color_join_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return
        (((int32_t)((r) & 0xff)) << 16)
        | (((int32_t)((g) & 0xff)) << 8)
        | (((int32_t)((b) & 0xff)))
        | COLOR_FLAG_RGB
    ;
}

static bool parse_color(const char *str, int32_t *val)
{
    size_t len = strlen(str);

    if (str[0] == '#') {
        if (len != 7) {
            return false;
        }
        for (size_t i = 1; i < len; i++) {
            if (hex_decode(str[i]) == -1) {
                return false;
            }
        }

        uint8_t r, g, b;
        int n = sscanf(str + 1, "%2" SCNx8 "%2" SCNx8 "%2" SCNx8, &r, &g, &b);
        if (n != 3) {
            return false;
        }
        *val = color_join_rgb(r, g, b);
        return true;
    }

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
            len != 5 ||
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

    bool light = false;
    if (len >= 8 && memcmp(str, "light", 5) == 0) {
        light = true;
        str += 5;
        len -= 5;
    }

    const int32_t c = lookup_color(str, len);
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
        int32_t val;
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

static CONST_FN int color_dist_sq (
    uint8_t R, uint8_t G, uint8_t B,
    uint8_t r, uint8_t g, uint8_t b
) {
    return (R - r) * (R - r) + (G - g) * (G - g) + (B - b) * (B - b);
}

// Convert RGB color component (0-255) to nearest xterm color cube index (0-5)
static CONST_FN uint8_t rgb_component_to_nearest_cube_index(uint8_t c)
{
    if (c < 48) {
        return 0;
    }
    if (c < 114) {
        return 1;
    }
    return (c - 35) / 40;
}

// Convert xterm color cube index to corresponding RGB color component
static uint8_t cube_index_to_rgb_component(uint8_t idx)
{
    static const uint8_t color_stops[6] = {
        0x00, 0x5f, 0x87,
        0xaf, 0xd7, 0xff
    };
    return color_stops[idx];
}

static PURE uint8_t color_rgb_to_256(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t r_idx = rgb_component_to_nearest_cube_index(r);
    uint8_t r_stop = cube_index_to_rgb_component(r_idx);

    uint8_t g_idx = rgb_component_to_nearest_cube_index(g);
    uint8_t g_stop = cube_index_to_rgb_component(g_idx);

    uint8_t b_idx = rgb_component_to_nearest_cube_index(b);
    uint8_t b_stop = cube_index_to_rgb_component(b_idx);

    if (r_stop == r && g_stop == g && b_stop == b) {
        // Exact match
        return 16 + (36 * r_idx) + (6 * g_idx) + b_idx;
    }

    // Calculate closest gray
    int gray_avg = (r + g + b) / 3;
    int gray_idx = (gray_avg > 238) ? 23 : ((gray_avg - 3) / 10);
    int gray = 8 + (10 * gray_idx);

    int rgb_distance = color_dist_sq(r_stop, g_stop, b_stop, r, g, b);
    int gray_distance = color_dist_sq(gray, gray, gray, r, g, b);
    if (gray_distance < rgb_distance) {
        // Gray is closest match
        return 232 + gray_idx;
    } else {
        // RGB cube color is closest match
        return 16 + (36 * r_idx) + (6 * g_idx) + b_idx;
    }
}

static PURE uint8_t color_256_to_16(uint8_t color)
{
    static const uint8_t table[256] = {
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
         0,  4,  4,  4, 12, 12,  2,  6,  4,  4, 12, 12,  2,  2,  6,  4,
        12, 12,  2,  2,  2,  6, 12, 12, 10, 10, 10, 10, 14, 12, 10, 10,
        10, 10, 10, 14,  1,  5,  4,  4, 12, 12,  3,  8,  4,  4, 12, 12,
         2,  2,  6,  4, 12, 12,  2,  2,  2,  6, 12, 12, 10, 10, 10, 10,
        14, 12, 10, 10, 10, 10, 10, 14,  1,  1,  5,  4, 12, 12,  1,  1,
         5,  4, 12, 12,  3,  3,  8,  4, 12, 12,  2,  2,  2,  6, 12, 12,
        10, 10, 10, 10, 14, 12, 10, 10, 10, 10, 10, 14,  1,  1,  1,  5,
        12, 12,  1,  1,  1,  5, 12, 12,  1,  1,  1,  5, 12, 12,  3,  3,
         3,  7, 12, 12, 10, 10, 10, 10, 14, 12, 10, 10, 10, 10, 10, 14,
         9,  9,  9,  9, 13, 12,  9,  9,  9,  9, 13, 12,  9,  9,  9,  9,
        13, 12,  9,  9,  9,  9, 13, 12, 11, 11, 11, 11,  7, 12, 10, 10,
        10, 10, 10, 14,  9,  9,  9,  9,  9, 13,  9,  9,  9,  9,  9, 13,
         9,  9,  9,  9,  9, 13,  9,  9,  9,  9,  9, 13,  9,  9,  9,  9,
         9, 13, 11, 11, 11, 11, 11, 15,  0,  0,  0,  0,  0,  0,  8,  8,
         8,  8,  8,  8,  7,  7,  7,  7,  7,  7, 15, 15, 15, 15, 15, 15
    };
    return table[color];
}

static uint8_t color_any_to_256(int32_t color)
{
    BUG_ON(color < 0);
    if (color & COLOR_FLAG_RGB) {
        uint8_t r, g, b;
        color_split_rgb(color, &r, &g, &b);
        return color_rgb_to_256(r, g, b);
    }
    BUG_ON(color > 255);
    return color;
}

static uint8_t color_any_to_16(int32_t color)
{
    return color_256_to_16(color_any_to_256(color));
}

static uint8_t color_any_to_8(int32_t color)
{
    return color_any_to_16(color) % 8;
}

int32_t convert_color_to_nearest_supported(int32_t color)
{
    if (color < 0) {
        return color;
    }
    switch (terminal.color_type) {
    case TERM_0_COLOR: return COLOR_DEFAULT;
    case TERM_8_COLOR: return color_any_to_8(color);
    case TERM_16_COLOR: return color_any_to_16(color);
    case TERM_256_COLOR: return color_any_to_256(color);
    case TERM_TRUE_COLOR: return color;
    }
    BUG("unexpected terminal.color_type value");
    UNREACHABLE();
}
