#include "terminfo.h"
#include "../debug.h"
#include "../util/macros.h"

#ifndef TERMINFO_DISABLE

#include <stdint.h>
#include <string.h>
#include "key.h"
#include "output.h"
#include "terminal.h"
#include "xterm.h"
#include "../util/ascii.h"
#include "../util/str-util.h"
#include "../util/string-view.h"

#define KEY(c, k) { \
    .code = (c), \
    .code_length = (sizeof(c) - 1), \
    .key = (k) \
}

#define XKEYS(p, key) \
    KEY(p,     key | MOD_SHIFT), \
    KEY(p "3", key | MOD_META), \
    KEY(p "4", key | MOD_SHIFT | MOD_META), \
    KEY(p "5", key | MOD_CTRL), \
    KEY(p "6", key | MOD_SHIFT | MOD_CTRL), \
    KEY(p "7", key | MOD_META | MOD_CTRL), \
    KEY(p "8", key | MOD_SHIFT | MOD_META | MOD_CTRL)

static struct {
    const char *clear;
    const char *cup;
    const char *el;
    const char *setab;
    const char *setaf;
    const char *sgr;
} terminfo;

static struct TermKeyMap {
    const char *code;
    uint32_t code_length;
    KeyCode key;
} keymap[] = {
    KEY("kcuu1", KEY_UP),
    KEY("kcud1", KEY_DOWN),
    KEY("kcub1", KEY_LEFT),
    KEY("kcuf1", KEY_RIGHT),
    KEY("kdch1", KEY_DELETE),
    KEY("kpp", KEY_PAGE_UP),
    KEY("knp", KEY_PAGE_DOWN),
    KEY("khome", KEY_HOME),
    KEY("kend", KEY_END),
    KEY("kich1", KEY_INSERT),
    KEY("kcbt", MOD_SHIFT | '\t'),
    KEY("kf1", KEY_F1),
    KEY("kf2", KEY_F2),
    KEY("kf3", KEY_F3),
    KEY("kf4", KEY_F4),
    KEY("kf5", KEY_F5),
    KEY("kf6", KEY_F6),
    KEY("kf7", KEY_F7),
    KEY("kf8", KEY_F8),
    KEY("kf9", KEY_F9),
    KEY("kf10", KEY_F10),
    KEY("kf11", KEY_F11),
    KEY("kf12", KEY_F12),
    XKEYS("kUP", KEY_UP),
    XKEYS("kDN", KEY_DOWN),
    XKEYS("kLFT", KEY_LEFT),
    XKEYS("kRIT", KEY_RIGHT),
    XKEYS("kDC", KEY_DELETE),
    XKEYS("kPRV", KEY_PAGE_UP),
    XKEYS("kNXT", KEY_PAGE_DOWN),
    XKEYS("kHOM", KEY_HOME),
    XKEYS("kEND", KEY_END),
};

static_assert(ARRAY_COUNT(keymap) == 23 + (9 * 7));

static size_t keymap_length = 0;

static ssize_t parse_key_from_keymap(const char *buf, size_t fill, KeyCode *key)
{
    bool possibly_truncated = false;
    for (size_t i = 0; i < keymap_length; i++) {
        const struct TermKeyMap *const km = &keymap[i];
        const char *const keycode = km->code;
        const size_t len = km->code_length;
        BUG_ON(keycode == NULL);
        BUG_ON(len == 0);
        if (len > fill) {
            // This might be a truncated escape sequence
            if (
                possibly_truncated == false
                && memcmp(keycode, buf, fill) == 0
            ) {
                possibly_truncated = true;
            }
            continue;
        }
        if (memcmp(keycode, buf, len) != 0) {
            continue;
        }
        *key = km->key;
        return len;
    }
    return possibly_truncated ? -1 : 0;
}

// These are normally declared in the <curses.h> and <term.h> headers.
// They are not included here because of the insane number of unprefixed
// symbols they declare and because of previous bugs caused by using them.
int setupterm(const char *term, int filedes, int *errret);
int tigetflag(const char *capname);
int tigetnum(const char *capname);
char *tigetstr(const char *capname);
int tputs(const char *str, int affcnt, int (*putc_fn)(int));
char *tparm(const char*, long, long, long, long, long, long, long, long, long);
#define tparm_1(str, p1) tparm(str, p1, 0, 0, 0, 0, 0, 0, 0, 0)
#define tparm_2(str, p1, p2) tparm(str, p1, p2, 0, 0, 0, 0, 0, 0, 0)

static char *get_terminfo_string(const char *capname)
{
    char *str = tigetstr(capname);
    if (str == (char *)-1) {
        // Not a string cap (bug?)
        return NULL;
    }
    // NULL = canceled or absent
    return str;
}

static StringView get_terminfo_string_view(const char *capname)
{
    return string_view_from_cstring(get_terminfo_string(capname));
}

static bool get_terminfo_flag(const char *capname)
{
    switch (tigetflag(capname)) {
    case -1: // Not a boolean capability
    case 0: // Canceled or absent
        return false;
    }
    return true;
}

static int tputs_putc(int ch)
{
    term_add_byte(ch);
    return ch;
}

static void tputs_control_code(StringView code)
{
    if (code.length) {
        tputs(code.data, 1, tputs_putc);
    }
}

static void tputs_clear_screen(void)
{
    if (terminfo.clear) {
        tputs(terminfo.clear, terminal.height, tputs_putc);
    }
}

static void tputs_clear_to_eol(void)
{
    if (terminfo.el) {
        tputs(terminfo.el, 1, tputs_putc);
    }
}

static void tputs_move_cursor(int x, int y)
{
    if (terminfo.cup) {
        const char *seq = tparm_2(terminfo.cup, y, x);
        if (seq) {
            tputs(seq, 1, tputs_putc);
        }
    }
}

static bool attr_is_set(const TermColor *color, unsigned int attr)
{
    if (!(color->attr & attr)) {
        return false;
    } else if (terminal.ncv_attributes & attr) {
        // Terminal only allows attr when not using colors
        return color->fg == COLOR_DEFAULT && color->bg == COLOR_DEFAULT;
    }
    return true;
}

static void tputs_set_color(const TermColor *color)
{
    if (same_color(color, &obuf.color)) {
        return;
    }

    if (terminfo.sgr) {
        const char *attrs = tparm (
            terminfo.sgr,
            0, // p1 = "standout" (unused)
            attr_is_set(color, ATTR_UNDERLINE),
            attr_is_set(color, ATTR_REVERSE),
            attr_is_set(color, ATTR_BLINK),
            attr_is_set(color, ATTR_DIM),
            attr_is_set(color, ATTR_BOLD),
            attr_is_set(color, ATTR_INVIS),
            0, // p8 = "protect" (unused)
            0  // p9 = "altcharset" (unused)
        );
        tputs(attrs, 1, tputs_putc);
    }

    TermColor c = *color;
    if (terminfo.setaf && c.fg >= 0) {
        const char *seq = tparm_1(terminfo.setaf, c.fg);
        if (seq) {
            tputs(seq, 1, tputs_putc);
        }
    }
    if (terminfo.setab && c.bg >= 0) {
        const char *seq = tparm_1(terminfo.setab, c.bg);
        if (seq) {
            tputs(seq, 1, tputs_putc);
        }
    }

    obuf.color = *color;
}

static unsigned int convert_ncv_flags_to_attrs(unsigned int ncv)
{
    // These flags should have values equal to their terminfo
    // counterparts:
    static_assert(ATTR_UNDERLINE == 2);
    static_assert(ATTR_REVERSE == 4);
    static_assert(ATTR_BLINK == 8);
    static_assert(ATTR_DIM == 16);
    static_assert(ATTR_BOLD == 32);
    static_assert(ATTR_INVIS == 64);

    // Mask flags to supported, common subset
    unsigned int attrs = ncv & (
        ATTR_UNDERLINE | ATTR_REVERSE | ATTR_BLINK
        | ATTR_DIM | ATTR_BOLD | ATTR_INVIS
    );

    // Italic is a special case; it occupies bit 16 in terminfo
    // but bit 7 here
    if (ncv & 0x8000) {
        attrs |= ATTR_ITALIC;
    }

    return attrs;
}

bool term_init_terminfo(const char *term)
{
    // Initialize terminfo database (or call exit(3) on failure)
    setupterm(term, 1, (int*)0);

    terminal.put_control_code = &tputs_control_code;
    terminal.clear_screen = &tputs_clear_screen;
    terminal.clear_to_eol = &tputs_clear_to_eol;
    terminal.set_color = &tputs_set_color;
    terminal.move_cursor = &tputs_move_cursor;

    if (get_terminfo_flag("nxon")) {
        term_init_fail (
            "TERM type '%s' not supported: 'nxon' flag is set",
            term
        );
    }

    terminfo.cup = get_terminfo_string("cup");
    if (terminfo.cup == NULL) {
        term_init_fail (
            "TERM type '%s' not supported: no 'cup' capability",
            term
        );
    }

    terminfo.clear = get_terminfo_string("clear");
    terminfo.el = get_terminfo_string("el");
    terminfo.setab = get_terminfo_string("setab");
    terminfo.setaf = get_terminfo_string("setaf");
    terminfo.sgr = get_terminfo_string("sgr");

    terminal.back_color_erase = get_terminfo_flag("bce");
    terminal.width = tigetnum("cols");
    terminal.height = tigetnum("lines");

    switch (tigetnum("colors")) {
    case 16777216:
        // Just use the built-in xterm_set_color() function if true color
        // support is indicated. This bypasses tputs(3), but no true color
        // terminal in existence actually depends on archaic tputs(3)
        // features (like e.g. baudrate-dependant padding).
        terminal.color_type = TERM_TRUE_COLOR;
        terminal.set_color = &xterm_set_color;
        break;
    case 256:
        terminal.color_type = TERM_256_COLOR;
        break;
    case 16:
        terminal.color_type = TERM_16_COLOR;
        break;
    case 88:
    case 8:
        terminal.color_type = TERM_8_COLOR;
        break;
    default:
        terminal.color_type = TERM_0_COLOR;
        break;
    }

    const int ncv = tigetnum("ncv");
    if (ncv <= 0) {
        terminal.ncv_attributes = 0;
    } else {
        terminal.ncv_attributes = convert_ncv_flags_to_attrs(ncv);
    }

    terminal.control_codes = (TermControlCodes) {
        .reset_colors = get_terminfo_string_view("op"),
        .reset_attrs = get_terminfo_string_view("sgr0"),
        .keypad_off = get_terminfo_string_view("rmkx"),
        .keypad_on = get_terminfo_string_view("smkx"),
        .cup_mode_off = get_terminfo_string_view("rmcup"),
        .cup_mode_on = get_terminfo_string_view("smcup"),
        .show_cursor = get_terminfo_string_view("cnorm"),
        .hide_cursor = get_terminfo_string_view("civis")
    };

    bool xterm_compatible_key_codes = true;

    for (size_t i = 0; i < ARRAY_COUNT(keymap); i++) {
        const char *const code = get_terminfo_string(keymap[i].code);
        if (code && code[0] != '\0') {
            const size_t code_len = strlen(code);
            const KeyCode key = keymap[i].key;
            keymap[keymap_length++] = (struct TermKeyMap) {
                .code = code,
                .code_length = code_len,
                .key = key
            };
            KeyCode parsed_key;
            const ssize_t parsed_len = xterm_parse_key(code, code_len, &parsed_key);
            if (parsed_len <= 0 || parsed_key != key) {
                xterm_compatible_key_codes = false;
            }
        }
    }

    if (!xterm_compatible_key_codes) {
        terminal.parse_key_sequence = &parse_key_from_keymap;
    }

    return true; // Initialization succeeded
}

#else

bool term_init_terminfo(const char* UNUSED_ARG(term))
{
    return false; // terminfo not available
}

#endif // ifndef TERMINFO_DISABLE
