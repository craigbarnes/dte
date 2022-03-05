#include <errno.h>
#include <stdint.h>
#include <string.h>
#include "terminal.h"
#include "ecma48.h"
#include "linux.h"
#include "mode.h"
#include "osc52.h"
#include "rxvt.h"
#include "xterm.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/str-util.h"

typedef struct {
    const char name[12];
    unsigned int name_len : 4;
    unsigned int color_type : 3; // TermColorCapabilityType
    unsigned int ncv_attrs : 5; // TermColor attributes (see "ncv" in terminfo(5))
    unsigned int features : 20; // TermFeatureFlags
} TermEntry;

enum {
    // Short aliases for TermFeatureFlags:
    BCE = TFLAG_BACK_COLOR_ERASE,
    REP = TFLAG_ECMA48_REPEAT,
    TITLE = TFLAG_SET_WINDOW_TITLE,
    RXVT = TFLAG_RXVT,
    LINUX = TFLAG_LINUX,
    OSC52 = TFLAG_OSC52_COPY,
    METAESC = TFLAG_META_ESC,
    KITTYKBD = TFLAG_KITTY_KEYBOARD,
    ITERM2 = TFLAG_ITERM2,
};

enum {
    // Short aliases for TermColor attributes:
    UL = ATTR_UNDERLINE,
    REV = ATTR_REVERSE,
    DIM = ATTR_DIM,
};

static const TermEntry terms[] = {
    {"Eterm", 5, TERM_8_COLOR, 0, BCE},
    {"alacritty", 9, TERM_TRUE_COLOR, 0, BCE | REP | OSC52},
    {"ansi", 4, TERM_8_COLOR, UL, 0},
    {"ansiterm", 8, TERM_0_COLOR, 0, 0},
    {"aterm", 5, TERM_8_COLOR, 0, BCE},
    {"contour", 7, TERM_TRUE_COLOR, 0, BCE | REP | TITLE | OSC52},
    {"cx", 2, TERM_8_COLOR, 0, 0},
    {"cx100", 5, TERM_8_COLOR, 0, 0},
    {"cygwin", 6, TERM_8_COLOR, 0, 0},
    {"cygwinB19", 9, TERM_8_COLOR, UL, 0},
    {"cygwinDBG", 9, TERM_8_COLOR, UL, 0},
    {"decansi", 7, TERM_8_COLOR, 0, 0},
    {"domterm", 7, TERM_8_COLOR, 0, BCE},
    {"dtterm", 6, TERM_8_COLOR, 0, 0},
    {"dvtm", 4, TERM_8_COLOR, 0, 0},
    {"fbterm", 6, TERM_256_COLOR, DIM | UL, BCE},
    {"foot", 4, TERM_TRUE_COLOR, 0, BCE | REP | TITLE | OSC52 | KITTYKBD},
    {"hurd", 4, TERM_8_COLOR, DIM | UL, BCE},
    {"iTerm.app", 9, TERM_256_COLOR, 0, BCE},
    {"iTerm2.app", 10, TERM_256_COLOR, 0, BCE | TITLE | OSC52 | ITERM2},
    {"iterm", 5, TERM_256_COLOR, 0, BCE},
    {"iterm2", 6, TERM_256_COLOR, 0, BCE | TITLE | OSC52 | ITERM2},
    {"jfbterm", 7, TERM_8_COLOR, DIM | UL, BCE},
    {"kitty", 5, TERM_TRUE_COLOR, 0, TITLE | OSC52 | KITTYKBD},
    {"kon", 3, TERM_8_COLOR, DIM | UL, BCE},
    {"kon2", 4, TERM_8_COLOR, DIM | UL, BCE},
    {"konsole", 7, TERM_8_COLOR, 0, BCE},
    {"kterm", 5, TERM_8_COLOR, 0, 0},
    {"linux", 5, TERM_8_COLOR, DIM | UL, LINUX | BCE},
    {"mgt", 3, TERM_8_COLOR, 0, BCE},
    {"mintty", 6, TERM_8_COLOR, 0, BCE | REP | TITLE | OSC52},
    {"mlterm", 6, TERM_8_COLOR, 0, TITLE},
    {"mlterm2", 7, TERM_8_COLOR, 0, TITLE},
    {"mlterm3", 7, TERM_8_COLOR, 0, TITLE},
    {"mrxvt", 5, TERM_8_COLOR, 0, RXVT | BCE | TITLE | OSC52},
    {"pcansi", 6, TERM_8_COLOR, UL, 0},
    {"putty", 5, TERM_8_COLOR, DIM | REV | UL, BCE},
    {"rxvt", 4, TERM_8_COLOR, 0, RXVT | BCE | TITLE | OSC52},
    {"screen", 6, TERM_8_COLOR, 0, TITLE | OSC52},
    {"st", 2, TERM_8_COLOR, 0, BCE | OSC52},
    {"stterm", 6, TERM_8_COLOR, 0, BCE | OSC52},
    {"teken", 5, TERM_8_COLOR, DIM | REV, BCE},
    {"terminator", 10, TERM_256_COLOR, 0, BCE | TITLE},
    {"termite", 7, TERM_8_COLOR, 0, TITLE},
    {"tmux", 4, TERM_8_COLOR, 0, TITLE | OSC52},
    {"wezterm", 7, TERM_TRUE_COLOR, 0, BCE | REP | TITLE},
    {"xfce", 4, TERM_8_COLOR, 0, BCE | TITLE},
    {"xterm", 5, TERM_8_COLOR, 0, BCE | TITLE | OSC52 | METAESC},
    {"xterm.js", 8, TERM_8_COLOR, 0, BCE},
};

static const struct {
    const char suffix[11];
    uint8_t suffix_len;
    TermColorCapabilityType color_type;
} color_suffixes[] = {
    {"direct", 6, TERM_TRUE_COLOR},
    {"256color", 8, TERM_256_COLOR},
    {"16color", 7, TERM_16_COLOR},
    {"mono", 4, TERM_0_COLOR},
    {"m", 1, TERM_0_COLOR},
};

Terminal terminal = {
    .color_type = TERM_8_COLOR,
    .width = 80,
    .height = 24,
    .parse_key_sequence = &xterm_parse_key,
    .set_color = &ecma48_set_color,
    .repeat_byte = &term_repeat_byte,
    .control_codes = {
        .keypad_off = STRING_VIEW("\033[?1l\033>"),
        .keypad_on = STRING_VIEW("\033[?1h\033="),
        .cup_mode_off = STRING_VIEW("\033[?1049l"),
        .cup_mode_on = STRING_VIEW("\033[?1049h"),
        .hide_cursor = STRING_VIEW("\033[?25l"),
        .show_cursor = STRING_VIEW("\033[?25h"),
        .set_title_begin = STRING_VIEW_INIT,
        .set_title_end = STRING_VIEW_INIT,
        .save_title = STRING_VIEW_INIT,
        .restore_title = STRING_VIEW_INIT,
    }
};

static int term_name_compare(const void *key, const void *elem)
{
    const StringView *prefix = key;
    const TermEntry *entry = elem;
    size_t cmplen = MIN(prefix->length, entry->name_len);
    int r = memcmp(prefix->data, entry->name, cmplen);
    return r ? r : (int)prefix->length - entry->name_len;
}

UNITTEST {
    CHECK_BSEARCH_ARRAY(terms, name, strcmp);
    StringView k = STRING_VIEW("xtermz");
    BUG_ON(BSEARCH(&k, terms, term_name_compare));
    k.length--;
    BUG_ON(!BSEARCH(&k, terms, term_name_compare));
    k.length--;
    BUG_ON(BSEARCH(&k, terms, term_name_compare));
}

void term_init(const char *term)
{
    BUG_ON(term[0] == '\0');

    // Strip phony "xterm-" prefix used by certain terminals
    const char *real_term = term;
    if (str_has_prefix(term, "xterm-")) {
        const char *str = term + STRLEN("xterm-");
        if (str_has_prefix(str, "kitty") || str_has_prefix(str, "termite")) {
            real_term = str;
        }
    }

    // Extract the "root name" from $TERM, as defined by terminfo(5).
    // This is the initial part of the string up to the first hyphen.
    size_t pos = 0;
    size_t rtlen = strlen(real_term);
    StringView name = get_delim(real_term, &pos, rtlen, '-');

    // Look up the root name in the list of known terminals
    const TermEntry *entry = BSEARCH(&name, terms, term_name_compare);
    TermControlCodes *tcc = &terminal.control_codes;
    if (entry) {
        TermFeatureFlags features = entry->features;
        terminal.features = features;
        terminal.color_type = entry->color_type;
        terminal.ncv_attributes = entry->ncv_attrs;
        if (features & REP) {
            terminal.repeat_byte = ecma48_repeat_byte;
        }
        if (features & TITLE) {
            tcc->save_title = strview_from_cstring("\033[22;2t");
            tcc->restore_title = strview_from_cstring("\033[23;2t");
            tcc->set_title_begin = strview_from_cstring("\033]2;");
            tcc->set_title_end = strview_from_cstring("\033\\");
        }
        if (features & RXVT) {
            terminal.parse_key_sequence = rxvt_parse_key;
        } else if (features & LINUX) {
            terminal.parse_key_sequence = linux_parse_key;
        }
        const int n = (int)name.length;
        DEBUG_LOG("using built-in terminal support for '%.*s'", n, name.data);
    }

    if (xstreq(getenv("COLORTERM"), "truecolor")) {
        terminal.color_type = TERM_TRUE_COLOR;
        DEBUG_LOG("true color support detected ($COLORTERM)");
    }
    if (terminal.color_type == TERM_TRUE_COLOR) {
        return;
    }

    while (pos < rtlen) {
        const StringView str = get_delim(real_term, &pos, rtlen, '-');
        for (size_t i = 0; i < ARRAYLEN(color_suffixes); i++) {
            const char *suffix = color_suffixes[i].suffix;
            size_t len = color_suffixes[i].suffix_len;
            if (strview_equal_strn(&str, suffix, len)) {
                terminal.color_type = color_suffixes[i].color_type;
                DEBUG_LOG("color type detected from $TERM suffix '-%s'", suffix);
                return;
            }
        }
    }
}

void term_enable_private_modes(const Terminal *term, TermOutputBuffer *obuf)
{
    TermFeatureFlags features = term->features;
    if (features & METAESC) {
        term_add_literal(obuf, "\033[?1036;1039s\033[?1036;1039h");
    }
    if (features & KITTYKBD) {
        term_add_literal(obuf, "\033[>5u");
    } else if (features & ITERM2) {
        term_add_literal(obuf, "\033[>1u");
    } else {
        // Try to use "modifyOtherKeys" mode
        term_add_literal(obuf, "\033[>4;1m");
    }
    term_add_strview(obuf, term->control_codes.keypad_on);
}

void term_restore_private_modes(const Terminal *term, TermOutputBuffer *obuf)
{
    TermFeatureFlags features = term->features;
    if (features & METAESC) {
        term_add_literal(obuf, "\033[?1036;1039r");
    }
    if (features & (KITTYKBD | ITERM2)) {
        term_add_literal(obuf, "\033[<u");
    } else {
        term_add_literal(obuf, "\033[>4m");
    }
    term_add_strview(obuf, term->control_codes.keypad_off);
}
