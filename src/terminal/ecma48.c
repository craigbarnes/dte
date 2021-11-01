#include <stdint.h>
#include "ecma48.h"
#include "terminal.h"
#include "util/ascii.h"
#include "util/macros.h"

void ecma48_repeat_byte(TermOutputBuffer *obuf, char ch, size_t count)
{
    if (!ascii_isprint(ch) || count < 6 || count > 30000) {
        term_repeat_byte(obuf, ch, count);
        return;
    }
    term_add_byte(obuf, ch);
    term_add_literal(obuf, "\033[");
    term_add_uint(obuf, count - 1);
    term_add_byte(obuf, 'b');
}

static void do_set_color(TermOutputBuffer *obuf, int32_t color, char ch)
{
    if (color < 0) {
        return;
    }

    term_add_byte(obuf, ';');
    term_add_byte(obuf, ch);

    if (likely(color < 8)) {
        term_add_byte(obuf, '0' + color);
    } else if (color < 256) {
        term_add_literal(obuf, "8;5;");
        term_add_uint(obuf, color);
    } else {
        uint8_t r, g, b;
        color_split_rgb(color, &r, &g, &b);
        term_add_literal(obuf, "8;2;");
        term_add_uint(obuf, r);
        term_add_byte(obuf, ';');
        term_add_uint(obuf, g);
        term_add_byte(obuf, ';');
        term_add_uint(obuf, b);
    }
}

static bool attr_is_set(const TermColor *color, unsigned int attr)
{
    if (color->attr & attr) {
        if (unlikely(terminal.ncv_attributes & attr)) {
            // Terminal only allows attr when not using colors
            return color->fg == COLOR_DEFAULT && color->bg == COLOR_DEFAULT;
        }
        return true;
    }
    return false;
}

void ecma48_set_color(TermOutputBuffer *obuf, const TermColor *color)
{
    static const struct {
        char code;
        unsigned int attr;
    } attr_map[] = {
        {'1', ATTR_BOLD},
        {'2', ATTR_DIM},
        {'3', ATTR_ITALIC},
        {'4', ATTR_UNDERLINE},
        {'5', ATTR_BLINK},
        {'7', ATTR_REVERSE},
        {'8', ATTR_INVIS},
        {'9', ATTR_STRIKETHROUGH}
    };

    term_add_literal(obuf, "\033[0");

    for (size_t i = 0; i < ARRAY_COUNT(attr_map); i++) {
        if (attr_is_set(color, attr_map[i].attr)) {
            term_add_byte(obuf, ';');
            term_add_byte(obuf, attr_map[i].code);
        }
    }

    do_set_color(obuf, color->fg, '3');
    do_set_color(obuf, color->bg, '4');
    term_add_byte(obuf, 'm');
}
