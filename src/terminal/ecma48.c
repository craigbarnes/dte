#include <stdio.h>
#include <termios.h>
#undef CTRL // macro from sys/ttydefaults.h clashes with the one in key.h
#include "ecma48.h"
#include "output.h"
#include "terminfo.h"
#include "xterm.h"
#include "../util/macros.h"

static struct termios termios_save;

void term_raw(void)
{
    // See termios(3)
    struct termios termios;

    tcgetattr(0, &termios);
    termios_save = termios;

    // Disable buffering
    // Disable echo
    // Disable generation of signals (free some control keys)
    termios.c_lflag &= ~(ICANON | ECHO | ISIG);

    // Disable CR to NL conversion (differentiate ^J from enter)
    // Disable flow control (free ^Q and ^S)
    termios.c_iflag &= ~(ICRNL | IXON | IXOFF);

    // Read at least 1 char on each read()
    termios.c_cc[VMIN] = 1;

    // Read blocks until there are MIN(VMIN, requested) bytes available
    termios.c_cc[VTIME] = 0;

    tcsetattr(0, 0, &termios);
}

void term_cooked(void)
{
    tcsetattr(0, 0, &termios_save);
}

void ecma48_clear_to_eol(void)
{
    buf_add_bytes("\033[K", 3);
}

void ecma48_move_cursor(int x, int y)
{
    if (x < 0 || x >= 999 || y < 0 || y >= 999) {
        return;
    }
    static char buf[16];
    const int len = snprintf (
        buf,
        11, // == strlen("\033[998;998H") + 1 (for NUL, to avoid truncation)
        "\033[%u;%uH",
        // x and y are zero-based
        ((unsigned int)y) + 1,
        ((unsigned int)x) + 1
    );
    static_assert(6 == STRLEN("\033[0;0H"));
    //BUG_ON(len < 6);
    static_assert(10 == STRLEN("\033[998;998H"));
    //BUG_ON(len > 10);
    buf_add_bytes(buf, (size_t)len);
}

void ecma48_set_color(const TermColor *const color)
{
    if (same_color(color, &obuf.color)) {
        return;
    }

    // Max 14 bytes ("E[0;1;7;30;40m")
    char buf[32] = "\033[0";
    size_t i = 3;
    TermColor c = *color;

    if (c.fg >= 8 && c.fg <= 15) {
        c.attr |= ATTR_BOLD;
        c.fg &= 7;
    }

    // TODO: convert colors > 15 to closest supported color

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
    buf_add_bytes(buf, i);
    obuf.color = *color;
}

static void no_op(void) {}
static void no_op_s(const char* UNUSED_ARG(s)) {}

TerminalInfo terminal = {
    .max_colors = 8,
    .width = 80,
    .height = 24,
    .raw = &term_raw,
    .cooked = &term_cooked,
    .parse_key_sequence = &xterm_parse_key,
    .clear_to_eol = &ecma48_clear_to_eol,
    .set_color = &ecma48_set_color,
    .move_cursor = &ecma48_move_cursor,
    .save_title = &no_op,
    .restore_title = &no_op,
    .set_title = &no_op_s,
    .control_codes = &(TermControlCodes) {
        .reset_colors = "\033[39;49m",
        .reset_attrs = "\033[0m",
    }
};
