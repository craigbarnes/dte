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
    {"alacritty", 9, TERM_8_COLOR, 0, BCE | REP},
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
    {"foot", 4, TERM_256_COLOR, 0, BCE | REP | TITLE},
    {"hurd", 4, TERM_8_COLOR, 18, BCE},
    {"iTerm.app", 9, TERM_256_COLOR, 0, BCE},
    {"iTerm2.app", 10, TERM_256_COLOR, 0, BCE | TITLE},
    {"iterm", 5, TERM_256_COLOR, 0, BCE},
    {"iterm2", 6, TERM_256_COLOR, 0, BCE | TITLE},
    {"jfbterm", 7, TERM_8_COLOR, 18, BCE},
    {"kitty", 5, TERM_256_COLOR, 0, TITLE},
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

static StringView str_split(const char *str, unsigned char delim)
{
    const char *end = strchr(str, delim);
    return string_view(str, end ? (size_t)(end - str) : strlen(str));
}

void term_init(void)
{
    const char *const term = getenv("TERM");
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
    StringView name = str_split(real_term, '-');

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
            terminal.save_title = &xterm_save_title;
            terminal.restore_title = &xterm_restore_title;
            terminal.set_title = &xterm_set_title;
        }
        if (streq(entry->name, "rxvt") || streq(entry->name, "mrxvt")) {
            terminal.parse_key_sequence = rxvt_parse_key;
        }
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
