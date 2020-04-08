#include "ecma48.h"
#include "output.h"
#include "../util/ascii.h"
#include "../util/macros.h"

void ecma48_clear_screen(void)
{
    term_add_literal (
        "\033[0m" // Reset colors and attributes
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
    term_xnprintf(64, "\033[%u;%uH", y + 1, x + 1);
}

void ecma48_set_color(const TermColor *color)
{
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
}

void ecma48_repeat_byte(char ch, size_t count)
{
    if (!ascii_isprint(ch) || count < 6 || count > 30000) {
        term_repeat_byte(ch, count);
        return;
    }
    term_xnprintf(16, "%c\033[%zub", ch, count - 1);
}
