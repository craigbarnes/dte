#ifndef TERMINAL_COLOR_H
#define TERMINAL_COLOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "terminal.h"
#include "util/macros.h"

#define COLOR_RGB(x) (COLOR_FLAG_RGB | (x))

enum {
    COLOR_INVALID = -3,
    COLOR_KEEP = -2,
    COLOR_DEFAULT = -1,
    COLOR_BLACK = 0,
    COLOR_RED = 1,
    COLOR_GREEN = 2,
    COLOR_YELLOW = 3,
    COLOR_BLUE = 4,
    COLOR_MAGENTA = 5,
    COLOR_CYAN = 6,
    COLOR_GRAY = 7,
    COLOR_DARKGRAY = 8,
    COLOR_LIGHTRED = 9,
    COLOR_LIGHTGREEN = 10,
    COLOR_LIGHTYELLOW = 11,
    COLOR_LIGHTBLUE = 12,
    COLOR_LIGHTMAGENTA = 13,
    COLOR_LIGHTCYAN = 14,
    COLOR_WHITE = 15,

    // This bit flag is used to allow 24-bit RGB colors to be differentiated
    // from basic colors (e.g. #000004 vs. COLOR_BLUE)
    COLOR_FLAG_RGB = INT32_C(1) << 24
};

static inline bool color_is_rgb(int32_t color)
{
    return !!(color & COLOR_FLAG_RGB);
}

static inline bool color_is_valid(int32_t color)
{
    bool palette = (color >= COLOR_KEEP && color <= 255);
    bool rgb = (color >= COLOR_RGB(0) && color <= COLOR_RGB(0xFFFFFF));
    return palette || rgb;
}

// Extract red channel from RGB color
static inline uint8_t color_r(int32_t rgb)
{
    return (rgb >> 16) & 0xFF;
}

// Extract green channel from RGB color
static inline uint8_t color_g(int32_t rgb)
{
    return (rgb >> 8) & 0xFF;
}

// Extract blue channel from RGB color
static inline uint8_t color_b(int32_t rgb)
{
    return rgb & 0xFF;
}

int32_t parse_rgb(const char *str, size_t len);
int32_t color_to_nearest(int32_t color, TermFeatureFlags flags, bool optimize);

#endif
