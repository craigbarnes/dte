#include <stdio.h>
#include "xterm.h"
#include "ecma48.h"
#include "output.h"

void xterm_save_title(void)
{
    buf_add_bytes("\033[22;2t", 7);
}

void xterm_restore_title(void)
{
    buf_add_bytes("\033[23;2t", 7);
}

void xterm_set_title(const char *title)
{
    buf_add_bytes("\033]2;", 4);
    buf_escape(title);
    buf_add_ch('\007');
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
        {'8', ATTR_INVIS}
    };

    if (same_color(color, &obuf.color)) {
        return;
    }

    // Max 36 bytes ("E[0;1;2;3;4;5;7;8;38;5;255;48;5;255m")
    char buf[64] = "\033[0";
    size_t i = 3;
    TermColor c = *color;

    for (size_t j = 0; j < ARRAY_COUNT(attr_map); j++) {
        if (c.attr & attr_map[j].attr) {
            buf[i++] = ';';
            buf[i++] = attr_map[j].code;
        }
    }

    if (c.fg >= 0) {
        const unsigned char fg = (unsigned char) c.fg;
        if (fg < 8) {
            buf[i++] = ';';
            buf[i++] = '3';
            buf[i++] = '0' + fg;
        } else {
            i += snprintf(&buf[i], 10, ";38;5;%u", fg);
        }
    }

    if (c.bg >= 0) {
        const unsigned char bg = (unsigned char) c.bg;
        if (bg < 8) {
            buf[i++] = ';';
            buf[i++] = '4';
            buf[i++] = '0' + bg;
        } else {
            i += snprintf(&buf[i], 10, ";48;5;%u", bg);
        }
    }

    buf[i++] = 'm';
    buf_add_bytes(buf, i);
    obuf.color = *color;
}

const TerminalInfo terminal_xterm = {
    .back_color_erase = true,
    .max_colors = 8,
    .width = 80,
    .height = 24,
    .raw = &term_raw,
    .cooked = &term_cooked,
    .parse_key_sequence = &xterm_parse_key,
    .clear_to_eol = &ecma48_clear_to_eol,
    .set_color = &xterm_set_color,
    .move_cursor = &ecma48_move_cursor,
    .repeat_byte = &buf_repeat_byte,
    .save_title = &xterm_save_title,
    .restore_title = &xterm_restore_title,
    .set_title = &xterm_set_title,
    .control_codes = {
        // https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
        .reset_colors = "\033[39;49m",
        .reset_attrs = "\033[0m",
        .keypad_off = "\033[?1l\033>",
        .keypad_on = "\033[?1h\033=",
        .cup_mode_off = "\033[?1049l",
        .cup_mode_on = "\033[?1049h",
        .hide_cursor = "\033[?25l",
        .show_cursor = "\033[?25h",
    }
};
