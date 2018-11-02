#include <stdio.h>
#include "xterm.h"
#include "ecma48.h"
#include "output.h"
#include "../debug.h"
#include "../util/xsnprintf.h"

void xterm_save_title(void)
{
    buf_add_literal("\033[22;2t");
}

void xterm_restore_title(void)
{
    buf_add_literal("\033[23;2t");
}

void xterm_set_title(const char *title)
{
    buf_add_literal("\033]2;");
    buf_add_bytes(title, strlen(title));
    buf_add_ch('\007');
}

static inline void do_set_color(char *buf, size_t *pos, int32_t color, char ch)
{
    if (color < 0) {
        return;
    }

    size_t i = *pos;
    buf[i++] = ';';
    buf[i++] = ch;

    if (color < 8) {
        buf[i++] = '0' + color;
    } else if (color < 256) {
        i += xsnprintf(buf + i, 8, "8;5;%u", (unsigned int) color);
    } else if (color & COLOR_FLAG_RGB) {
        uint8_t r, g, b;
        color_split_rgb(color, &r, &g, &b);
        i += xsnprintf(buf + i, 16, "8;2;%hhu;%hhu;%hhu", r, g, b);
    } else {
        BUG("color value shouldn't be > 255 when COLOR_FLAG_RGB bit is unset");
    }

    *pos = i;
}

void xterm_set_color(const TermColor *color)
{
    static const struct {
        char code;
        unsigned short attr;
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

    if (same_color(color, &obuf.color)) {
        return;
    }

    char buf[64] = "\033[0";
    size_t i = 3;

    static_assert(sizeof(buf) >= STRLEN("\033[0m")
        + (2 * ARRAY_COUNT(attr_map))
        + (2 * STRLEN(";38;2;255;255;255"))
    );

    for (size_t j = 0; j < ARRAY_COUNT(attr_map); j++) {
        if (color->attr & attr_map[j].attr) {
            buf[i++] = ';';
            buf[i++] = attr_map[j].code;
        }
    }

    do_set_color(buf, &i, color->fg, '3');
    do_set_color(buf, &i, color->bg, '4');

    buf[i++] = 'm';
    buf_add_bytes(buf, i);
    obuf.color = *color;
}

const Terminal xterm = {
    .back_color_erase = true,
    .color_type = TERM_8_COLOR,
    .width = 80,
    .height = 24,
    .raw = &term_raw,
    .cooked = &term_cooked,
    .parse_key_sequence = &xterm_parse_key,
    .put_control_code = &put_control_code,
    .clear_to_eol = &ecma48_clear_to_eol,
    .set_color = &xterm_set_color,
    .move_cursor = &ecma48_move_cursor,
    .repeat_byte = &buf_repeat_byte,
    .save_title = &xterm_save_title,
    .restore_title = &xterm_restore_title,
    .set_title = &xterm_set_title,
    .control_codes = {
        // https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
        .init = STRING_VIEW (
            "\033[?1036s" // Save "metaSendsEscape"
            "\033[?1036h" // Enable "metaSendsEscape"
        ),
        .deinit = STRING_VIEW (
            "\033[?1036r" // Restore "metaSendsEscape"
        ),
        .reset_colors = STRING_VIEW("\033[39;49m"),
        .reset_attrs = STRING_VIEW("\033[0m"),
        .keypad_off = STRING_VIEW("\033[?1l\033>"),
        .keypad_on = STRING_VIEW("\033[?1h\033="),
        .cup_mode_off = STRING_VIEW("\033[?1049l"),
        .cup_mode_on = STRING_VIEW("\033[?1049h"),
        .hide_cursor = STRING_VIEW("\033[?25l"),
        .show_cursor = STRING_VIEW("\033[?25h"),
    }
};
