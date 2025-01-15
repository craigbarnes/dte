#include "color.h"
#include "util/debug.h"
#include "util/strtonum.h"

int32_t parse_rgb(const char *str, size_t len)
{
    unsigned int mask = 0;
    unsigned int val = 0;

    if (len == 6) {
        for (size_t i = 0; i < len; i++) {
            unsigned int digit = hex_decode(str[i]);
            mask |= digit;
            val = val << 4 | digit;
        }
    } else if (len == 3) {
        for (size_t i = 0; i < len; i++) {
            unsigned int digit = hex_decode(str[i]);
            mask |= digit;
            val = val << 8 | digit << 4 | digit;
        }
    } else {
        return COLOR_INVALID;
    }

    return unlikely(mask & HEX_INVALID) ? COLOR_INVALID : COLOR_RGB(val);
}

// Calculate squared Euclidean distance between 2 RGB colors
static int color_distance (
    uint8_t R, uint8_t G, uint8_t B,
    uint8_t r, uint8_t g, uint8_t b
) {
    static_assert(INT_MAX >= 3 * 255 * 255);
    return ((R - r) * (R - r)) + ((G - g) * (G - g)) + ((B - b) * (B - b));
}

UNITTEST {
    // NOLINTBEGIN(bugprone-assert-side-effect)
    BUG_ON(color_distance(1,1,1, 1,0,1) != 1);
    BUG_ON(color_distance(100,0,0, 80,0,0) != 400);
    BUG_ON(color_distance(0,5,10, 5,0,2) != 25 + 25 + 64);
    BUG_ON(color_distance(0,0,0, 255,0,0) != 255 * 255);
    BUG_ON(color_distance(255,255,255, 0,0,0) != 255 * 255 * 3);
    // NOLINTEND(bugprone-assert-side-effect)
}

// Convert RGB color component (0-255) to nearest xterm color cube index (0-5).
// Color stops: 0, 95, 135, 175, 215, 255.
static unsigned int nearest_cube_index(uint8_t c)
{
    unsigned int a = (c < 80) ? MIN(c, 7) : 35;
    return (c - a) / 40;
}

UNITTEST {
    // NOLINTBEGIN(bugprone-assert-side-effect)
    BUG_ON(nearest_cube_index(0) != 0);
    BUG_ON(nearest_cube_index(46) != 0);
    BUG_ON(nearest_cube_index(47) != 1);
    BUG_ON(nearest_cube_index(114) != 1);
    BUG_ON(nearest_cube_index(115) != 2);
    BUG_ON(nearest_cube_index(170) != 3);
    BUG_ON(nearest_cube_index(255) != 5);
    BUG_ON(nearest_cube_index(255 - 20) != 5);
    BUG_ON(nearest_cube_index(255 - 21) != 4);
    // NOLINTEND(bugprone-assert-side-effect)
}

static uint8_t color_rgb_to_256(uint32_t color, bool *exact)
{
    BUG_ON(!color_is_rgb(color));
    uint8_t r = color_r(color);
    uint8_t g = color_g(color);
    uint8_t b = color_b(color);

    // Calculate closest 6x6x6 RGB cube color
    static const uint8_t color_stops[6] = {0, 95, 135, 175, 215, 255};
    uint8_t r_idx = nearest_cube_index(r);
    uint8_t g_idx = nearest_cube_index(g);
    uint8_t b_idx = nearest_cube_index(b);
    uint8_t r_stop = color_stops[r_idx];
    uint8_t g_stop = color_stops[g_idx];
    uint8_t b_stop = color_stops[b_idx];

    // Calculate closest gray
    int gray_avg = (r + g + b) / 3;
    int gray_idx = (gray_avg > 238) ? 23 : ((gray_avg - 3) / 10);
    int gray = 8 + (10 * gray_idx);

    // Calculate differences
    int rgb_distance = color_distance(r_stop, g_stop, b_stop, r, g, b);
    int gray_distance = color_distance(gray, gray, gray, r, g, b);

    if (gray_distance < rgb_distance) {
        // Gray is closest match
        *exact = (gray_distance == 0);
        return 232 + gray_idx;
    } else {
        // RGB cube color is closest match
        *exact = (rgb_distance == 0);
        return 16 + (36 * r_idx) + (6 * g_idx) + b_idx;
    }
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

// Quantize a color to the nearest value supported by the terminal
int32_t color_to_nearest(int32_t c, TermFeatureFlags flags, bool optimize)
{
    BUG_ON(!color_is_valid(c));
    BUG_ON(optimize && !(flags & TFLAG_TRUE_COLOR));

    // Note that higher color capabilities are taken as implying support for
    // the lower ones (because no terminal supports e.g. true color but not
    // palette colors)
    int32_t limit = COLOR_DEFAULT;
    if (flags & TFLAG_TRUE_COLOR) {
        limit = optimize ? 255 : COLOR_RGB(0xFFFFFF);
    } else if (flags & TFLAG_256_COLOR) {
        limit = 255;
    } else if (flags & TFLAG_16_COLOR) {
        limit = COLOR_WHITE;
    } else if (flags & TFLAG_8_COLOR) {
        limit = COLOR_GRAY;
    }

    if (likely(c <= limit)) {
        // Color is already within the supported range
        return c;
    }

    bool rgb = color_is_rgb(c);
    BUG_ON(optimize && !rgb);

    bool exact = true;
    int32_t tmp = rgb ? color_rgb_to_256(c, &exact) : c;
    c = (!(flags & TFLAG_TRUE_COLOR) || exact) ? tmp : c;
    c = (limit <= COLOR_WHITE) ? color_256_to_16(c) : c;
    c = (limit <= COLOR_GRAY) ? (c & 7) : c;
    c = (limit <= COLOR_DEFAULT) ? COLOR_DEFAULT : c;
    return c;
}
