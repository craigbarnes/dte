#include <stdio.h>
#include "xterm.h"
#include "ecma48.h"
#include "mode.h"
#include "output.h"
#include "../util/xsnprintf.h"

void xterm_save_title(void)
{
    term_add_literal("\033[22;2t");
}

void xterm_restore_title(void)
{
    term_add_literal("\033[23;2t");
}

void xterm_set_title(const char *title)
{
    term_add_literal("\033]2;");
    term_add_bytes(title, strlen(title));
    term_add_byte('\007');
}

static void do_set_color(int32_t color, char ch)
{
    if (color < 0) {
        return;
    }

    if (color < 8) {
        term_add_byte(';');
        term_add_byte(ch);
        term_add_byte('0' + color);
    } else if (color < 256) {
        term_xnprintf(16, ";%c8;5;%hhu", ch, (uint8_t)color);
    } else {
        uint8_t r, g, b;
        color_split_rgb(color, &r, &g, &b);
        term_xnprintf(32, ";%c8;2;%hhu;%hhu;%hhu", ch, r, g, b);
    }
}

void xterm_set_color(const TermColor *color)
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

    term_add_literal("\033[0");

    for (size_t j = 0; j < ARRAY_COUNT(attr_map); j++) {
        if (color->attr & attr_map[j].attr) {
            term_add_byte(';');
            term_add_byte(attr_map[j].code);
        }
    }

    do_set_color(color->fg, '3');
    do_set_color(color->bg, '4');
    term_add_byte('m');
}
