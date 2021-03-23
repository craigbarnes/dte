#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "terminal.h"
#include "ecma48.h"
#include "mode.h"
#include "output.h"
#include "rxvt.h"
#include "xterm.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/macros.h"
#include "util/str-util.h"

typedef enum {
    BCE = 0x01, // Can erase with specific background color (back color erase)
    REP = 0x02, // Supports ECMA-48 "REP" (repeat character)
    TITLE = 0x04, // Supports xterm control codes for setting window title
} TermFlags;

typedef struct {
    const char name[12];
    uint8_t name_len;
    uint8_t color_type;
    uint8_t ncv_attributes;
    uint8_t flags;
} TermEntry;

static const TermEntry terms[] = {
    {"Eterm", 5, TERM_8_COLOR, 0, BCE},
    {"alacritty", 9, TERM_TRUE_COLOR, 0, BCE | REP},
    {"ansi", 4, TERM_8_COLOR, 3, 0},
    {"ansiterm", 8, TERM_0_COLOR, 0, 0},
    {"aterm", 5, TERM_8_COLOR, 0, BCE},
    {"cx", 2, TERM_8_COLOR, 0, 0},
    {"cx100", 5, TERM_8_COLOR, 0, 0},
    {"cygwin", 6, TERM_8_COLOR, 0, 0},
    {"cygwinB19", 9, TERM_8_COLOR, 3, 0},
    {"cygwinDBG", 9, TERM_8_COLOR, 3, 0},
    {"decansi", 7, TERM_8_COLOR, 0, 0},
    {"domterm", 7, TERM_8_COLOR, 0, BCE},
    {"dtterm", 6, TERM_8_COLOR, 0, 0},
    {"dvtm", 4, TERM_8_COLOR, 0, 0},
    {"fbterm", 6, TERM_256_COLOR, 18, BCE},
    {"foot", 4, TERM_TRUE_COLOR, 0, BCE | REP | TITLE},
    {"hurd", 4, TERM_8_COLOR, 18, BCE},
    {"iTerm.app", 9, TERM_256_COLOR, 0, BCE},
    {"iTerm2.app", 10, TERM_256_COLOR, 0, BCE | TITLE},
    {"iterm", 5, TERM_256_COLOR, 0, BCE},
    {"iterm2", 6, TERM_256_COLOR, 0, BCE | TITLE},
    {"jfbterm", 7, TERM_8_COLOR, 18, BCE},
    {"kitty", 5, TERM_TRUE_COLOR, 0, TITLE},
    {"kon", 3, TERM_8_COLOR, 18, BCE},
    {"kon2", 4, TERM_8_COLOR, 18, BCE},
    {"konsole", 7, TERM_8_COLOR, 0, BCE},
    {"kterm", 5, TERM_8_COLOR, 0, 0},
    {"linux", 5, TERM_8_COLOR, 18, BCE},
    {"mgt", 3, TERM_8_COLOR, 0, BCE},
    {"mintty", 6, TERM_8_COLOR, 0, BCE | REP | TITLE},
    {"mlterm", 6, TERM_8_COLOR, 0, TITLE},
    {"mlterm2", 7, TERM_8_COLOR, 0, TITLE},
    {"mlterm3", 7, TERM_8_COLOR, 0, TITLE},
    {"mrxvt", 5, TERM_8_COLOR, 0, BCE | TITLE},
    {"pcansi", 6, TERM_8_COLOR, 3, 0},
    {"putty", 5, TERM_8_COLOR, 22, BCE},
    {"rxvt", 4, TERM_8_COLOR, 0, BCE | TITLE},
    {"screen", 6, TERM_8_COLOR, 0, TITLE},
    {"st", 2, TERM_8_COLOR, 0, BCE},
    {"stterm", 6, TERM_8_COLOR, 0, BCE},
    {"teken", 5, TERM_8_COLOR, 21, BCE},
    {"terminator", 10, TERM_256_COLOR, 0, BCE | TITLE},
    {"termite", 7, TERM_8_COLOR, 0, TITLE},
    {"tmux", 4, TERM_8_COLOR, 0, TITLE},
    {"xfce", 4, TERM_8_COLOR, 0, BCE | TITLE},
    {"xterm", 5, TERM_8_COLOR, 0, BCE | TITLE},
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
    .back_color_erase = false,
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
    .control_codes = {
        // https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
        .init = STRING_VIEW (
            // 1036 = metaSendsEscape
            // 1039 = altSendsEscape
            // 2017 = kitty "full" keyboard mode
            "\033[?1036;1039s" // Save
            "\033[?1036;1039;2017h" // Enable
        ),
        .deinit = STRING_VIEW (
            "\033[?1036;1039r" // Restore
            "\033[?2017l" // Disable
        ),
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
    size_t cmplen = MIN(prefix->length, (size_t)entry->name_len);
    int res = memcmp(prefix->data, entry->name, cmplen);
    if (res == 0) {
        return (int)prefix->length - (int)entry->name_len;
    }
    return res;
}

UNITTEST {
    static const StringView tests[] = {
        STRING_VIEW("Eterm"),
        STRING_VIEW("alacritty"),
        STRING_VIEW("ansi"),
        STRING_VIEW("mlterm"),
        STRING_VIEW("mlterm2"),
        STRING_VIEW("mlterm3"),
        STRING_VIEW("st"),
        STRING_VIEW("stterm"),
        STRING_VIEW("xterm"),
    };
    CHECK_BSEARCH_ARRAY(terms, name, strcmp);
    for (size_t i = 0; i < ARRAY_COUNT(tests); i++) {
        const TermEntry *entry = BSEARCH(&tests[i], terms, term_name_compare);
        BUG_ON(!entry);
        BUG_ON(!strview_equal_strn(&tests[i], entry->name, entry->name_len));
    }
}

void term_init(void)
{
    const char *const term = getenv("TERM");
    if (!term || term[0] == '\0') {
        init_error("'TERM' not set");
    }

    if (!term_mode_init()) {
        init_error("tcgetattr: %s", strerror(errno));
    }

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
    if (entry) {
        terminal.color_type = entry->color_type;
        terminal.ncv_attributes = entry->ncv_attributes;
        terminal.back_color_erase = !!(entry->flags & BCE);
        if (entry->flags & REP) {
            terminal.repeat_byte = ecma48_repeat_byte;
        }
        if (entry->flags & TITLE) {
            terminal.control_codes.save_title = strview_from_cstring("\033[22;2t");
            terminal.control_codes.restore_title = strview_from_cstring("\033[23;2t");
            terminal.control_codes.set_title_begin = strview_from_cstring("\033]2;");
            terminal.control_codes.set_title_end = strview_from_cstring("\033\\");
        }
        if (streq(entry->name, "rxvt") || streq(entry->name, "mrxvt")) {
            terminal.parse_key_sequence = rxvt_parse_key;
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
        for (size_t i = 0; i < ARRAY_COUNT(color_suffixes); i++) {
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
