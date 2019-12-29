#include "ecma48.h"
#include "mode.h"
#include "no-op.h"
#include "output.h"
#include "xterm.h"
#include "../util/ascii.h"
#include "../util/macros.h"
#include "../util/xsnprintf.h"

void ecma48_clear_screen(void)
{
    term_add_literal (
        "\033[H"  // Move cursor to 1,1 (done only to mimic terminfo(5) "clear")
        "\033[2J" // Clear whole screen (regardless of cursor position)
    );
}

void ecma48_clear_to_eol(void)
{
    term_add_literal("\033[K");
}

void ecma48_move_cursor(unsigned int x, unsigned int y)
{
    char buf[64];
    size_t n = xsnprintf (
        buf,
        sizeof buf,
        "\033[%u;%uH",
        // x and y are zero-based
        ((unsigned int)y) + 1,
        ((unsigned int)x) + 1
    );
    term_add_bytes(buf, n);
}

void ecma48_set_color(const TermColor *const color)
{
    if (same_color(color, &obuf.color)) {
        return;
    }

    char buf[16] = "\033[0";
    size_t i = 3;
    static_assert(sizeof(buf) >= STRLEN("\033[0;1;7;30;40m"));

    TermColor c = *color;
    if (c.attr & ATTR_BOLD) {
        buf[i++] = ';';
        buf[i++] = '1';
    }
    if (c.attr & ATTR_REVERSE) {
        buf[i++] = ';';
        buf[i++] = '7';
    }

    if (c.fg >= 0 && c.fg < 8) {
        buf[i++] = ';';
        buf[i++] = '3';
        buf[i++] = '0' + (char) c.fg;
    }

    if (c.bg >= 0 && c.bg < 8) {
        buf[i++] = ';';
        buf[i++] = '4';
        buf[i++] = '0' + (char) c.bg;
    }

    buf[i++] = 'm';
    term_add_bytes(buf, i);
    obuf.color = *color;
}

void ecma48_repeat_byte(char ch, size_t count)
{
    if (!ascii_isprint(ch) || count < 6 || count > 30000) {
        term_repeat_byte(ch, count);
        return;
    }
    char buf[64];
    size_t n = xsnprintf(buf, sizeof buf, "%c\033[%zub", ch, count - 1);
    term_add_bytes(buf, n);
}

Terminal terminal = {
    .color_type = TERM_8_COLOR,
    .width = 80,
    .height = 24,
    .parse_key_sequence = &xterm_parse_key,
    .put_control_code = &term_add_string_view,
    .clear_screen = &ecma48_clear_screen,
    .clear_to_eol = &ecma48_clear_to_eol,
    .set_color = &ecma48_set_color,
    .move_cursor = &ecma48_move_cursor,
    .repeat_byte = &term_repeat_byte,
    .save_title = &no_op,
    .restore_title = &no_op,
    .set_title = &no_op_s,
    .control_codes = {
        .init = STRING_VIEW_INIT,
        .deinit = STRING_VIEW_INIT,
        .reset_colors = STRING_VIEW("\033[39;49m"),
        .reset_attrs = STRING_VIEW("\033[0m"),
        .keypad_off = STRING_VIEW_INIT,
        .keypad_on = STRING_VIEW_INIT,
        .cup_mode_off = STRING_VIEW_INIT,
        .cup_mode_on = STRING_VIEW_INIT,
        .show_cursor = STRING_VIEW_INIT,
        .hide_cursor = STRING_VIEW_INIT,
    }
};
