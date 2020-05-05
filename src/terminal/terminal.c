#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "terminal.h"
#include "ecma48.h"
#include "mode.h"
#include "no-op.h"
#include "output.h"
#include "rxvt.h"
#include "terminfo.h"
#include "xterm.h"
#include "debug.h"
#include "util/macros.h"
#include "util/str-util.h"

typedef struct {
    const char name[12];
    uint8_t name_len;
    uint8_t color_type;
    uint8_t ncv_attributes;
    bool back_color_erase;
} TermEntry;

static const TermEntry terms[] = {
    {"Eterm", 5, TERM_8_COLOR, 0, true},
    {"alacritty", 9, TERM_8_COLOR, 0, true},
    {"ansi", 4, TERM_8_COLOR, 3, false},
    {"ansiterm", 8, TERM_0_COLOR, 0, false},
    {"aterm", 5, TERM_8_COLOR, 0, true},
    {"cx", 2, TERM_8_COLOR, 0, false},
    {"cx100", 5, TERM_8_COLOR, 0, false},
    {"cygwin", 6, TERM_8_COLOR, 0, false},
    {"cygwinB19", 9, TERM_8_COLOR, 3, false},
    {"cygwinDBG", 9, TERM_8_COLOR, 3, false},
    {"decansi", 7, TERM_8_COLOR, 0, false},
    {"dtterm", 6, TERM_8_COLOR, 0, false},
    {"dvtm", 4, TERM_8_COLOR, 0, false},
    {"fbterm", 6, TERM_256_COLOR, 18, true},
    {"hurd", 4, TERM_8_COLOR, 18, true},
    {"iTerm.app", 9, TERM_256_COLOR, 0, true},
    {"iTerm2.app", 10, TERM_256_COLOR, 0, true},
    {"iterm", 5, TERM_256_COLOR, 0, true},
    {"iterm2", 6, TERM_256_COLOR, 0, true},
    {"jfbterm", 7, TERM_8_COLOR, 18, true},
    {"kitty", 5, TERM_256_COLOR, 0, false},
    {"kon", 3, TERM_8_COLOR, 18, true},
    {"kon2", 4, TERM_8_COLOR, 18, true},
    {"konsole", 7, TERM_8_COLOR, 0, true},
    {"kterm", 5, TERM_8_COLOR, 0, false},
    {"linux", 5, TERM_8_COLOR, 18, true},
    {"mgt", 3, TERM_8_COLOR, 0, true},
    {"mintty", 6, TERM_8_COLOR, 0, true},
    {"mlterm", 6, TERM_8_COLOR, 0, false},
    {"mlterm2", 7, TERM_8_COLOR, 0, false},
    {"mlterm3", 7, TERM_8_COLOR, 0, false},
    {"mrxvt", 5, TERM_8_COLOR, 0, true},
    {"pcansi", 6, TERM_8_COLOR, 3, false},
    {"putty", 5, TERM_8_COLOR, 22, true},
    {"rxvt", 4, TERM_8_COLOR, 0, true},
    {"screen", 6, TERM_8_COLOR, 0, false},
    {"st", 2, TERM_8_COLOR, 0, true},
    {"stterm", 6, TERM_8_COLOR, 0, true},
    {"teken", 5, TERM_8_COLOR, 21, true},
    {"terminator", 10, TERM_256_COLOR, 0, true},
    {"termite", 7, TERM_8_COLOR, 0, false},
    {"tmux", 4, TERM_8_COLOR, 0, false},
    {"xfce", 4, TERM_8_COLOR, 0, true},
    {"xterm", 5, TERM_8_COLOR, 0, true},
};

static const struct {
    const char suffix[11];
    uint8_t suffix_len;
    TermColorCapabilityType color_type;
} color_suffixes[] = {
    {"-direct", 7, TERM_TRUE_COLOR},
    {"-256color", 9, TERM_256_COLOR},
    {"-16color", 8, TERM_16_COLOR},
    {"-mono", 5, TERM_0_COLOR},
    {"-m", 2, TERM_0_COLOR},
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
    .save_title = &no_op,
    .restore_title = &no_op,
    .set_title = &no_op_s,
    .control_codes = {
        // https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
        .init = STRING_VIEW (
            // 1036 = metaSendsEscape
            // 1039 = altSendsEscape
            "\033[?1036;1039s" // Save
            "\033[?1036;1039h" // Enable
        ),
        .deinit = STRING_VIEW (
            "\033[?1036;1039r" // Restore
        ),
        .keypad_off = STRING_VIEW("\033[?1l\033>"),
        .keypad_on = STRING_VIEW("\033[?1h\033="),
        .cup_mode_off = STRING_VIEW("\033[?1049l"),
        .cup_mode_on = STRING_VIEW("\033[?1049h"),
        .hide_cursor = STRING_VIEW("\033[?25l"),
        .show_cursor = STRING_VIEW("\033[?25h"),
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
    for (size_t i = 1; i < ARRAY_COUNT(terms); i++) {
        BUG_ON(terms[i].name[sizeof(terms[0].name) - 1] != '\0');
        BUG_ON(strcmp(terms[i - 1].name, terms[i].name) > 0);
    }
    for (size_t i = 0; i < ARRAY_COUNT(tests); i++) {
        const TermEntry *entry = BSEARCH(&tests[i], terms, term_name_compare);
        BUG_ON(!strview_equal_strn(&tests[i], entry->name, entry->name_len));
    }
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

    if (str_has_prefix(term, "rxvt-unicode")) {
        // urxvt can't be handled by the lookup table because it's a
        // special case and requires a custom KeyCode parser
        terminal.parse_key_sequence = rxvt_parse_key;
        terminal.ncv_attributes = 0;
        terminal.color_type = TERM_8_COLOR;
        terminal.back_color_erase = true;
        goto out;
    } else if (str_has_prefix(term, "xterm-kitty")) {
        // Having a hyphen in the "root" name goes against longstanding
        // naming conventions and precludes bsearch() by prefix
        terminal.color_type = TERM_256_COLOR;
        terminal.ncv_attributes = 0;
        terminal.back_color_erase = false;
        goto out;
    }

    // Extract the "root name" from $TERM, as defined by terminfo(5).
    // This is the initial part of the string up to the first hyphen.
    const char *dash = strchr(term, '-');
    size_t prefix_len = dash ? (size_t)(dash - term) : strlen(term);
    StringView prefix = string_view(term, prefix_len);

    // Look up the root name in the list of known terminals
    const TermEntry *entry = BSEARCH(&prefix, terms, term_name_compare);
    if (entry) {
        terminal.color_type = entry->color_type;
        terminal.ncv_attributes = entry->ncv_attributes;
        terminal.back_color_erase = entry->back_color_erase;
        goto out;
    }

    // Fall back to using the terminfo database, if available
    if (term_init_terminfo(term)) {
        return;
    }

out:
    if (xstreq(getenv("COLORTERM"), "truecolor")) {
        terminal.color_type = TERM_TRUE_COLOR;
    } else {
        for (size_t i = 0; i < ARRAY_COUNT(color_suffixes); i++) {
            const char *suffix = color_suffixes[i].suffix;
            const char *substr = strstr(term, suffix);
            const size_t n = color_suffixes[i].suffix_len;
            if (substr && (substr[n] == '-' || substr[n] == '\0')) {
                terminal.color_type = color_suffixes[i].color_type;
                break;
            }
        }
    }
}
