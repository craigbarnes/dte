#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "terminal.h"
#include "ecma48.h"
#include "mode.h"
#include "rxvt.h"
#include "terminfo.h"
#include "xterm.h"
#include "../debug.h"
#include "../util/macros.h"
#include "../util/str-util.h"

typedef enum {
    TERM_OTHER,
    TERM_LINUX,
    TERM_SCREEN,
    TERM_ST,
    TERM_TMUX,
    TERM_URXVT,
    TERM_XTERM,
    TERM_KITTY,
} TerminalType;

static TerminalType get_term_type(const char *term)
{
    static const struct {
        const char name[14];
        uint8_t name_len;
        uint8_t type;
    } builtin_terminals[] = {
        {STRN("xterm-kitty"), TERM_KITTY},
        {STRN("kitty"), TERM_KITTY},
        {STRN("xterm"), TERM_XTERM},
        {STRN("st"), TERM_ST},
        {STRN("stterm"), TERM_ST},
        {STRN("tmux"), TERM_TMUX},
        {STRN("screen"), TERM_SCREEN},
        {STRN("linux"), TERM_LINUX},
        {STRN("rxvt-unicode"), TERM_URXVT},
    };
    const size_t term_len = strlen(term);
    for (size_t i = 0; i < ARRAY_COUNT(builtin_terminals); i++) {
        const size_t n = builtin_terminals[i].name_len;
        if (term_len >= n && mem_equal(term, builtin_terminals[i].name, n)) {
            if (term[n] == '-' || term[n] == '\0') {
                return builtin_terminals[i].type;
            }
        }
    }
    return TERM_OTHER;
}

UNITTEST {
    BUG_ON(get_term_type("xterm") != TERM_XTERM);
    BUG_ON(get_term_type("xterm-kitty") != TERM_KITTY);
    BUG_ON(get_term_type("tmux") != TERM_TMUX);
    BUG_ON(get_term_type("st") != TERM_ST);
    BUG_ON(get_term_type("stterm") != TERM_ST);
    BUG_ON(get_term_type("linux") != TERM_LINUX);
    BUG_ON(get_term_type("xterm-256color") != TERM_XTERM);
    BUG_ON(get_term_type("screen-256color") != TERM_SCREEN);
    BUG_ON(get_term_type("x") != TERM_OTHER);
    BUG_ON(get_term_type("xter") != TERM_OTHER);
    BUG_ON(get_term_type("xtermz") != TERM_OTHER);
}

noreturn void term_init_fail(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    putc('\n', stderr);
    fflush(stderr);
    exit(1);
}

void term_init(void)
{
    const char *term = getenv("TERM");
    if (term == NULL || term[0] == '\0') {
        term_init_fail("'TERM' not set");
    }

    if (!term_mode_init()) {
        term_init_fail("tcgetattr: %s", strerror(errno));
    }

    if (getenv("DTE_FORCE_TERMINFO")) {
        if (term_init_terminfo(term)) {
            return;
        } else {
            term_init_fail("'DTE_FORCE_TERMINFO' set but terminfo not linked");
        }
    }

    switch (get_term_type(term)) {
    case TERM_XTERM:
        terminal = xterm;
        // terminal.repeat_byte = &ecma48_repeat_byte;
        break;
    case TERM_ST:
        terminal = xterm;
        break;
    case TERM_URXVT:
        terminal = xterm;
        terminal.parse_key_sequence = rxvt_parse_key;
        break;
    case TERM_TMUX:
    case TERM_SCREEN:
    case TERM_KITTY:
        terminal = xterm;
        terminal.back_color_erase = false;
        break;
    case TERM_LINUX:
        // Use the default Terminal and just change the control codes
        terminal.control_codes.hide_cursor = xterm.control_codes.hide_cursor;
        terminal.control_codes.show_cursor = xterm.control_codes.show_cursor;
        break;
    case TERM_OTHER:
        if (term_init_terminfo(term)) {
            return;
        }
        break;
    }

    const char *colorterm = getenv("COLORTERM");
    if (colorterm && streq(colorterm, "truecolor")) {
        terminal.color_type = TERM_TRUE_COLOR;
        return;
    }

    if (
        terminal.color_type < TERM_256_COLOR
        && str_has_suffix(term, "256color")
    ) {
        terminal.color_type = TERM_256_COLOR;
    } else if (str_has_suffix(term, "-direct")) {
        terminal.color_type = TERM_TRUE_COLOR;
    }
}
