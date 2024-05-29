#include <stdint.h>
#include <string.h>
#include "terminal.h"
#include "color.h"
#include "linux.h"
#include "output.h"
#include "parse.h"
#include "rxvt.h"
#include "util/array.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/log.h"
#include "util/str-util.h"

typedef struct {
    unsigned int ncv_attrs : 5; // TermStyle attributes (see "ncv" in terminfo(5))
    unsigned int features : 27; // TermFeatureFlags
} TermInfo;

typedef struct {
    const char name[11];
    uint8_t name_len;
    TermInfo info;
} TermEntry;

enum {
    // Short aliases for TermFeatureFlags:
    BCE = TFLAG_BACK_COLOR_ERASE,
    REP = TFLAG_ECMA48_REPEAT,
    TITLE = TFLAG_SET_WINDOW_TITLE,
    RXVT = TFLAG_RXVT,
    LINUX = TFLAG_LINUX,
    OSC52 = TFLAG_OSC52_COPY,
    KITTYKBD = TFLAG_KITTY_KEYBOARD,
    ITERM2 = TFLAG_ITERM2,
    SYNC = TFLAG_SYNC,
    C8 = TFLAG_8_COLOR,
    C16 = TFLAG_16_COLOR | C8,
    C256 = TFLAG_256_COLOR | C16,
    TC = TFLAG_TRUE_COLOR | C256,
};

enum {
    // Short aliases for TermStyle attributes:
    UL = ATTR_UNDERLINE,
    REV = ATTR_REVERSE,
    DIM = ATTR_DIM,
};

#define t(tname, ncv, feat) { \
    .name = tname, \
    .name_len = STRLEN(tname), \
    .info = { \
        .ncv_attrs = ncv, \
        .features = feat, \
    }, \
}

static const TermEntry terms[] = {
    t("Eterm", 0, C8 | BCE),
    t("alacritty", 0, TC | BCE | REP | OSC52 | SYNC),
    t("ansi", UL, C8),
    t("ansiterm", 0, 0),
    t("aterm", 0, C8 | BCE),
    t("contour", 0, TC | BCE | REP | TITLE | OSC52 | SYNC),
    t("cx", 0, C8),
    t("cx100", 0, C8),
    t("cygwin", 0, C8),
    t("cygwinB19", UL, C8),
    t("cygwinDBG", UL, C8),
    t("decansi", 0, C8),
    t("domterm", 0, C8 | BCE),
    t("dtterm", 0, C8),
    t("dvtm", 0, C8),
    t("fbterm", DIM | UL, C256 | BCE),
    t("foot", 0, TC | BCE | REP | TITLE | OSC52 | KITTYKBD | SYNC),
    t("ghostty", 0, TC | BCE | REP | TITLE | OSC52 | KITTYKBD | SYNC),
    t("hurd", DIM | UL, C8 | BCE),
    t("iTerm.app", 0, C256 | BCE),
    t("iTerm2.app", 0, C256 | BCE | TITLE | OSC52 | ITERM2 | SYNC),
    t("iterm", 0, C256 | BCE),
    t("iterm2", 0, C256 | BCE | TITLE | OSC52 | ITERM2 | SYNC),
    t("jfbterm", DIM | UL, C8 | BCE),
    t("kitty", 0, TC | TITLE | OSC52 | KITTYKBD | SYNC),
    t("kon", DIM | UL, C8 | BCE),
    t("kon2", DIM | UL, C8 | BCE),
    t("konsole", 0, C8 | BCE),
    t("kterm", 0, C8),
    t("linux", DIM | UL, C8 | LINUX | BCE),
    t("mgt", 0, C8 | BCE),
    t("mintty", 0, C8 | BCE | REP | TITLE | OSC52 | SYNC),
    t("mlterm", 0, C8 | TITLE),
    t("mlterm2", 0, C8 | TITLE),
    t("mlterm3", 0, C8 | TITLE),
    t("mrxvt", 0, C8 | RXVT | BCE | TITLE | OSC52),
    t("pcansi", UL, C8),
    t("putty", DIM | REV | UL, C8 | BCE),
    t("rio", 0, TC | BCE | REP | OSC52 | SYNC),
    t("rxvt", 0, C8 | RXVT | BCE | TITLE | OSC52),
    t("screen", 0, C8 | TITLE | OSC52),
    t("st", 0, C8 | BCE | OSC52),
    t("stterm", 0, C8 | BCE | OSC52),
    t("teken", DIM | REV, C8 | BCE),
    t("terminator", 0, C256 | BCE | TITLE),
    t("termite", 0, C8 | TITLE),
    // TODO: Add REP to tmux features, when it becomes safe to assume that
    // TERM=tmux* implies tmux >= 2.6 (see tmux commit 5fc0be50450e75)
    t("tmux", 0, C8 | TITLE | OSC52),
    t("wezterm", 0, TC | BCE | REP | TITLE | OSC52 | SYNC),
    t("xfce", 0, C8 | BCE | TITLE),
    // The real xterm supports ECMA-48 REP, but TERM=xterm* is used by too
    // many other terminals to safely add it here.
    t("xterm", 0, C8 | BCE | TITLE | OSC52),
    t("xterm.js", 0, C8 | BCE),
};

static const struct {
    const char suffix[9];
    uint8_t suffix_len;
    unsigned int flags; // TermFeatureFlags
} color_suffixes[] = {
    {"direct", 6, TC},
    {"256color", 8, C256},
    {"16color", 7, C16},
    {"mono", 4, 0},
    {"m", 1, 0},
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
    CHECK_STRUCT_ARRAY(color_suffixes, suffix);

    // NOLINTBEGIN(bugprone-assert-side-effect)
    StringView k = STRING_VIEW("xtermz");
    BUG_ON(BSEARCH(&k, terms, term_name_compare));
    k.length--;
    BUG_ON(!BSEARCH(&k, terms, term_name_compare));
    k.length--;
    BUG_ON(BSEARCH(&k, terms, term_name_compare));
    // NOLINTEND(bugprone-assert-side-effect)

    for (size_t i = 0; i < ARRAYLEN(terms); i++) {
        size_t len = strlen(terms[i].name);
        BUG_ON(terms[i].name_len != len);
    }

    for (size_t i = 0; i < ARRAYLEN(color_suffixes); i++) {
        size_t len = strlen(color_suffixes[i].suffix);
        BUG_ON(color_suffixes[i].suffix_len != len);
    }
}

// Extract the "root name" from $TERM, as defined by terminfo(5).
// This is the initial part of the string up to the first hyphen.
static StringView term_extract_name(const char *name, size_t len, size_t *pos)
{
    StringView root = get_delim(name, pos, len, '-');
    if (*pos >= len || !strview_equal_cstring(&root, "xterm")) {
        return root;
    }

    // Skip past phony "xterm-" prefix used by certain terminals
    size_t tmp = *pos;
    StringView word2 = get_delim(name, &tmp, len, '-');
    if (
        strview_equal_cstring(&word2, "kitty")
        || strview_equal_cstring(&word2, "termite")
        || strview_equal_cstring(&word2, "ghostty")
    ) {
        *pos = tmp;
        return word2;
    }

    return root;
}

static TermInfo term_get_info(const char *name, const char *colorterm)
{
    TermInfo info = {
        .features = TFLAG_8_COLOR,
        .ncv_attrs = 0,
    };

    if (!name || name[0] == '\0') {
        LOG_NOTICE("$TERM unset; skipping terminal info lookup");
        return info;
    }

    LOG_INFO("TERM=%s", name);

    size_t pos = 0;
    size_t name_len = strlen(name);
    StringView root_name = term_extract_name(name, name_len, &pos);

    // Look up the root name in the list of known terminals
    const TermEntry *entry = BSEARCH(&root_name, terms, term_name_compare);
    if (entry) {
        LOG_INFO("using built-in terminal info for '%s'", entry->name);
        info = entry->info;
    }

    if (colorterm) {
        if (streq(colorterm, "truecolor") || streq(colorterm, "24bit")) {
            info.features |= TC;
            LOG_INFO("24-bit color support detected (COLORTERM=%s)", colorterm);
        } else if (colorterm[0] != '\0') {
            LOG_WARNING("unknown $COLORTERM value: '%s'", colorterm);
        }
    }

    if (info.features & TFLAG_TRUE_COLOR) {
        return info;
    }

    while (pos < name_len) {
        const StringView str = get_delim(name, &pos, name_len, '-');
        for (size_t i = 0; i < ARRAYLEN(color_suffixes); i++) {
            const char *suffix = color_suffixes[i].suffix;
            size_t suffix_len = color_suffixes[i].suffix_len;
            if (strview_equal_strn(&str, suffix, suffix_len)) {
                TermFeatureFlags flags = color_suffixes[i].flags;
                if (flags == 0) {
                    info.features &= ~TC;
                    info.ncv_attrs = 0;
                } else {
                    info.features |= flags;
                }
                LOG_INFO("color type detected from $TERM suffix '-%s'", suffix);
                return info;
            }
        }
    }

    return info;
}

void term_init(Terminal *term, const char *name, const char *colorterm)
{
    TermInfo info = term_get_info(name, colorterm);
    term->features = info.features;
    term->width = 80;
    term->height = 24;
    term->ncv_attributes = info.ncv_attrs;

    if (info.features & RXVT) {
        term->parse_input = rxvt_parse_key;
    } else if (info.features & LINUX) {
        term->parse_input = linux_parse_key;
    } else {
        term->parse_input = term_parse_sequence;
    }
}

void term_enable_private_modes(Terminal *term)
{
    TermOutputBuffer *obuf = &term->obuf;
    TermFeatureFlags features = term->features;
    if (features & TFLAG_META_ESC) {
        term_put_literal(obuf, "\033[?1036h"); // DECSET 1036 (metaSendsEscape)
    }
    if (features & TFLAG_ALT_ESC) {
        term_put_literal(obuf, "\033[?1039h"); // DECSET 1039 (altSendsEscape)
    }

    if (features & KITTYKBD) {
        // https://sw.kovidgoyal.net/kitty/keyboard-protocol/#progressive-enhancement
        term_put_literal(obuf, "\033[>5u");
    } else if (features & ITERM2) {
        // https://gitlab.com/craigbarnes/dte/-/issues/130#note_864453071
        term_put_literal(obuf, "\033[>1u");
    } else {
        // Try to use "modifyOtherKeys" mode
        term_put_literal(obuf, "\033[>4;1m");
    }

    // Try to enable bracketed paste mode. This is done unconditionally,
    // since it should be ignored by terminals that don't recognize it
    // and we really want to enable it for terminals that support it but
    // are spoofing $TERM for whatever reason.

    // TODO: fix term_read_bracketed_paste() to handle end delimiters
    // that get split between 2 reads before re-enabling this
    //term_put_literal(obuf, "\033[?2004s\033[?2004h");
}

void term_restore_private_modes(Terminal *term)
{
    TermOutputBuffer *obuf = &term->obuf;
    TermFeatureFlags features = term->features;
    if (features & TFLAG_META_ESC) {
        term_put_literal(obuf, "\033[?1036l"); // DECRST 1036 (metaSendsEscape)
    }
    if (features & TFLAG_ALT_ESC) {
        term_put_literal(obuf, "\033[?1039l"); // DECRST 1039 (altSendsEscape)
    }
    if (features & (KITTYKBD | ITERM2)) {
        term_put_literal(obuf, "\033[<u");
    } else {
        term_put_literal(obuf, "\033[>4m");
    }
    //term_put_literal(obuf, "\033[?2004l\033[?2004r");
}

void term_restore_cursor_style(Terminal *term)
{
    // TODO: query the cursor style at startup and restore that
    // value instead of using CURSOR_DEFAULT (which basically
    // amounts to using the so-called "DECSCUSR 0 hack"
    static const TermCursorStyle reset = {
        .type = CURSOR_DEFAULT,
        .color = COLOR_DEFAULT,
    };

    term_set_cursor_style(term, reset);
}
