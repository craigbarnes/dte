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
    const char name[11];
    uint8_t name_len;
    TermFeatureFlags features;
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
    MOKEYS = TFLAG_MODIFY_OTHER_KEYS,
    SYNC = TFLAG_SYNC,
    NOQUERY1 = TFLAG_NO_QUERY_L1,
    NOQUERY3 = TFLAG_NO_QUERY_L3,
    BSCTRL = TFLAG_BS_CTRL_BACKSPACE, // Only useful if not superseded by KITTYKBD
    DELCTRL = TFLAG_DEL_CTRL_BACKSPACE, // Only useful if not superseded by KITTYKBD
    C8 = TFLAG_8_COLOR,
    C16 = TFLAG_16_COLOR | C8,
    C256 = TFLAG_256_COLOR | C16,
    TC = TFLAG_TRUE_COLOR | C256,
    NCVUL = TFLAG_NCV_UNDERLINE,
    NCVREV = TFLAG_NCV_REVERSE,
    NCVDIM = TFLAG_NCV_DIM,
};

#define t(tname, feat) { \
    .name = tname, \
    .name_len = STRLEN(tname), \
    .features = (TermFeatureFlags)feat, \
}

static const TermEntry terms[] = {
    t("Eterm", C8 | BCE),
    t("alacritty", TC | BCE | REP | OSC52 | SYNC),
    t("ansi", C8 | NCVUL),
    t("ansiterm", 0),
    t("aterm", C8 | BCE),
    t("contour", TC | BCE | REP | TITLE | OSC52 | SYNC),
    t("cx", C8),
    t("cx100", C8),
    t("cygwin", C8),
    t("cygwinB19", C8 | NCVUL),
    t("cygwinDBG", C8 | NCVUL),
    t("decansi", C8),
    t("domterm", C8 | BCE),
    t("dtterm", C8),
    t("dvtm", C8 | BSCTRL),
    t("fbterm", C256 | BCE | NCVUL | NCVDIM),
    t("foot", TC | BCE | REP | TITLE | OSC52 | KITTYKBD | SYNC),
    t("ghostty", TC | BCE | REP | TITLE | OSC52 | KITTYKBD | SYNC),
    t("hurd", C8 | BCE | NCVUL | NCVDIM),
    t("iTerm.app", C256 | BCE),
    t("iTerm2.app", C256 | BCE | TITLE | OSC52 | SYNC),
    t("iterm", C256 | BCE),
    t("iterm2", C256 | BCE | TITLE | OSC52 | SYNC),
    t("jfbterm", C8 | BCE | NCVUL | NCVDIM),
    t("kitty", TC | TITLE | OSC52 | KITTYKBD | SYNC),
    t("kon", C8 | BCE | NCVUL | NCVDIM),
    t("kon2", C8 | BCE | NCVUL | NCVDIM),
    t("konsole", C8 | BCE),
    t("kterm", C8),
    t("linux", C8 | LINUX | BCE | NCVUL | NCVDIM),
    t("mgt", C8 | BCE),
    t("mintty", C8 | BCE | REP | TITLE | OSC52 | SYNC),
    t("mlterm", C8 | TITLE),
    t("mlterm2", C8 | TITLE),
    t("mlterm3", C8 | TITLE),
    t("mrxvt", C8 | RXVT | BCE | TITLE | OSC52),
    t("pcansi", C8 | NCVUL),
    t("putty", C8 | BCE | NCVUL | NCVDIM | NCVREV), // TODO: BSCTRL?
    t("rio", TC | BCE | REP | OSC52 | SYNC),
    t("rxvt", C8 | RXVT | BCE | TITLE | OSC52 | BSCTRL),
    t("screen", C8 | TITLE | OSC52),
    t("st", C8 | BCE | OSC52 | BSCTRL),
    t("stterm", C8 | BCE | OSC52),
    t("teken", C8 | BCE | NCVDIM | NCVREV),
    t("terminator", C256 | BCE | TITLE | BSCTRL),
    t("termite", C8 | TITLE),
    t("tmux", C8 | TITLE | OSC52 | NOQUERY3 | BSCTRL), // See also: parse_dcs_query_reply()
    t("vt220", NOQUERY1), // Used by cu(1) and picocom(1), which wrongly handle queries
    t("wezterm", TC | BCE | REP | TITLE | OSC52 | SYNC | BSCTRL),
    t("xfce", C8 | BCE | TITLE),
    // The real xterm supports ECMA-48 REP, but TERM=xterm* is used by too
    // many other terminals to safely add it here.
    // See also: parse_xtgettcap_reply()
    t("xterm", C8 | BCE | TITLE | OSC52),
    t("xterm.js", C8 | BCE),
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
    CHECK_BSEARCH_ARRAY(terms, name);
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
        const char *name = terms[i].name;
        size_t len = strlen(name);
        BUG_ON(terms[i].name_len != len);
        TermFeatureFlags imode_flags = KITTYKBD | BSCTRL | DELCTRL;
        TermFeatureFlags masked = terms[i].features & imode_flags;
        if (masked && !IS_POWER_OF_2(masked)) {
            BUG("TermEntry '%s' has multiple mutually exclusive flags", name);
        }
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

static TermFeatureFlags term_get_features(const char *name, const char *colorterm)
{
    TermFeatureFlags features = TFLAG_8_COLOR;
    if (!name || name[0] == '\0') {
        LOG_NOTICE("$TERM unset; skipping terminal info lookup");
        return features;
    }

    LOG_INFO("TERM=%s", name);

    size_t pos = 0;
    size_t name_len = strlen(name);
    StringView root_name = term_extract_name(name, name_len, &pos);

    // Look up the root name in the list of known terminals
    const TermEntry *entry = BSEARCH(&root_name, terms, term_name_compare);
    if (entry) {
        LOG_INFO("using built-in terminal info for '%s'", entry->name);
        features = entry->features;
    }

    if (colorterm) {
        if (streq(colorterm, "truecolor") || streq(colorterm, "24bit")) {
            features |= TC;
            LOG_INFO("24-bit color support detected (COLORTERM=%s)", colorterm);
        } else if (colorterm[0] != '\0') {
            LOG_WARNING("unknown $COLORTERM value: '%s'", colorterm);
        }
    }

    if (features & TFLAG_TRUE_COLOR) {
        return features;
    }

    while (pos < name_len) {
        const StringView str = get_delim(name, &pos, name_len, '-');
        for (size_t i = 0; i < ARRAYLEN(color_suffixes); i++) {
            const char *suffix = color_suffixes[i].suffix;
            size_t suffix_len = color_suffixes[i].suffix_len;
            if (strview_equal_strn(&str, suffix, suffix_len)) {
                TermFeatureFlags color_features = color_suffixes[i].flags;
                if (color_features == 0) {
                    features &= ~(TC | C256 | C16 | C8 | NCVUL | NCVREV | NCVDIM);
                } else {
                    features |= color_features;
                }
                LOG_INFO("color type detected from $TERM suffix '-%s'", suffix);
                return features;
            }
        }
    }

    return features;
}

void term_init(Terminal *term, const char *name, const char *colorterm)
{
    TermFeatureFlags features = term_get_features(name, colorterm);
    term->features = features;
    term->width = 80;
    term->height = 24;

    term->ncv_attributes =
        (features & NCVUL) ? ATTR_UNDERLINE : 0
        | (features & NCVDIM) ? ATTR_DIM : 0
        | (features & NCVREV) ? ATTR_REVERSE : 0
    ;

    if (features & RXVT) {
        term->parse_input = rxvt_parse_key;
    } else if (features & LINUX) {
        term->parse_input = linux_parse_key;
    } else {
        term->parse_input = term_parse_sequence;
    }
}

void term_enable_private_modes(Terminal *term)
{
    TermOutputBuffer *obuf = &term->obuf;
    TermFeatureFlags features = term->features;

    // Note that changes to some of the sequences below may require
    // corresponding updates to handle_query_reply()

    if (features & TFLAG_META_ESC) {
        term_put_literal(obuf, "\033[?1036h"); // DECSET 1036 (metaSendsEscape)
    }
    if (features & TFLAG_ALT_ESC) {
        term_put_literal(obuf, "\033[?1039h"); // DECSET 1039 (altSendsEscape)
    }

    if (features & KITTYKBD) {
        // https://sw.kovidgoyal.net/kitty/keyboard-protocol/#progressive-enhancement
        term_put_literal(obuf, "\033[>5u");
    } else if (features & MOKEYS) {
        // Try to use "modifyOtherKeys" mode (level 2 or 1)
        term_put_literal(obuf, "\033[>4;1m\033[>4;2m");
    }

    // Try to enable bracketed paste mode. This is done unconditionally,
    // since it should be ignored by terminals that don't recognize it
    // and we really want to enable it for terminals that support it but
    // are spoofing $TERM for whatever reason.
    term_put_literal(obuf, "\033[?2004s\033[?2004h");
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
    if (features & KITTYKBD) {
        term_put_literal(obuf, "\033[<u");
    } else if (features & MOKEYS) {
        term_put_literal(obuf, "\033[>4m");
    }
    term_put_literal(obuf, "\033[?2004l\033[?2004r");
}

void term_restore_cursor_style(Terminal *term)
{
    // TODO: Query the cursor style at startup (using DECRQSS DECSCUSR)
    // and restore the value provided in the reply (if any), instead
    // of using CURSOR_DEFAULT (which basically amounts to using the
    // so-called "DECSCUSR 0 hack")
    static const TermCursorStyle reset = {
        .type = CURSOR_DEFAULT,
        .color = COLOR_DEFAULT,
    };

    term_set_cursor_style(term, reset);
}
