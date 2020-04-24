#include "terminfo.h"

#ifndef TERMINFO_DISABLE

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ecma48.h"
#include "key.h"
#include "no-op.h"
#include "output.h"
#include "terminal.h"
#include "xterm.h"
#include "debug.h"
#include "util/str-util.h"
#include "util/string-view.h"

static struct TermInfo {
    const char *clear;
    const char *cup;
    const char *el;
    const char *setab;
    const char *setaf;
    const char *sgr;
    const char *sgr0;
} terminfo;

static size_t keymap_length = 0;

#define KEY(c, k) { \
    .code = (c), \
    .code_length = STRLEN(c), \
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

// This array initially contains mappings of terminfo(5) key names
// to KeyCode values. During initialization, each name is queried
// from the terminfo database and the array is overwritten with the
// corresponding escape sequences. The keymap_length variable above
// stores the resulting number of entries.
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

static ssize_t parse_key_from_keymap(const char *buf, size_t fill, KeyCode *key)
{
    static_assert(ARRAY_COUNT(keymap) == 23 + (9 * 7));
    bool possibly_truncated = false;
    for (size_t i = 0; i < keymap_length; i++) {
        const struct TermKeyMap *km = &keymap[i];
        const char *keycode = km->code;
        const size_t len = km->code_length;
        BUG_ON(keycode == NULL);
        BUG_ON(len == 0);
        if (len > fill) {
            // This might be a truncated escape sequence
            if (!possibly_truncated && mem_equal(keycode, buf, fill)) {
                possibly_truncated = true;
            }
            continue;
        }
        if (!mem_equal(keycode, buf, len)) {
            continue;
        }
        *key = km->key;
        return len;
    }
    return possibly_truncated ? -1 : 0;
}

// These functions are normally declared in the <curses.h> and <term.h>
// headers. They aren't included here because of the insane number of
// unhygienic macros they contain and because of previous bugs caused
// by using them.
int setupterm(const char *term, int filedes, int *errret);
int tigetflag(const char *capname);
int tigetnum(const char *capname);
char *tigetstr(const char *capname);
int tputs(const char *str, int affcnt, int (*putc_fn)(int));
char *tparm(const char*, long, long, long, long, long, long, long, long, long);

static const char *get_terminfo_string(const char *capname)
{
    const char *str = tigetstr(capname);
    return (str == (char*)-1) ? NULL : str;
}

static StringView get_terminfo_string_view(const char *capname)
{
    return strview_from_cstring(get_terminfo_string(capname));
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

static const char *xtparm1(const char *str, long p1)
{
    return str ? tparm(str, p1, 0, 0, 0, 0, 0, 0, 0, 0) : NULL;
}

static const char *xtparm2(const char *str, long p1, long p2)
{
    return str ? tparm(str, p1, p2, 0, 0, 0, 0, 0, 0, 0) : NULL;
}

static int xtputs_putc(int ch)
{
    term_add_byte(ch);
    return ch;
}

static void xtputs(const char *str, int lines_affected)
{
    if (str) {
        tputs(str, lines_affected, xtputs_putc);
    }
}

static void put_control_code(StringView code)
{
    xtputs(code.data, 1);
}

static void clear_screen(void)
{
    xtputs(terminfo.sgr0, 1);
    xtputs(terminfo.clear, terminal.height);
}

static void clear_to_eol(void)
{
    xtputs(terminfo.el, 1);
}

static void move_cursor(unsigned int x, unsigned int y)
{
    xtputs(xtparm2(terminfo.cup, (long)y, (long)x), 1);
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

static void set_color(const TermColor *color)
{
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
        xtputs(attrs, 1);
    }

    TermColor c = *color;
    if (c.fg >= 0) {
        xtputs(xtparm1(terminfo.setaf, c.fg), 1);
    }
    if (c.bg >= 0) {
        xtputs(xtparm1(terminfo.setab, c.bg), 1);
    }
}

static unsigned int get_ncv_flags_as_attrs(void)
{
    // These flags should have values equal to their terminfo
    // counterparts:
    static_assert(ATTR_UNDERLINE == 2);
    static_assert(ATTR_REVERSE == 4);
    static_assert(ATTR_BLINK == 8);
    static_assert(ATTR_DIM == 16);
    static_assert(ATTR_BOLD == 32);
    static_assert(ATTR_INVIS == 64);

    const int ncv = tigetnum("ncv");
    if (ncv <= 0) {
        return 0;
    }

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
    setupterm(term, STDOUT_FILENO, NULL);

    terminfo = (struct TermInfo) {
        .clear = get_terminfo_string("clear"),
        .cup = get_terminfo_string("cup"),
        .el = get_terminfo_string("el"),
        .setab = get_terminfo_string("setab"),
        .setaf = get_terminfo_string("setaf"),
        .sgr = get_terminfo_string("sgr"),
        .sgr0 = get_terminfo_string("sgr0"),
    };

    if (get_terminfo_flag("nxon")) {
        term_init_fail("TERM='%s' not supported: 'nxon' flag is set", term);
    }

    if (get_terminfo_flag("hz")) {
        term_init_fail("TERM='%s' not supported: 'hz' flag is set", term);
    }

    if (terminfo.cup == NULL) {
        term_init_fail("TERM='%s' not supported: no 'cup' capability", term);
    }

    int width = tigetnum("cols");
    int height = tigetnum("lines");

    terminal = (Terminal) {
        .back_color_erase = get_terminfo_flag("bce"),
        .color_type = TERM_0_COLOR,
        .width = (width > 2) ? width : 80,
        .height = (height > 2) ? height : 24,
        .ncv_attributes = get_ncv_flags_as_attrs(),
        .parse_key_sequence = &xterm_parse_key,
        .put_control_code = &put_control_code,
        .clear_screen = &clear_screen,
        .clear_to_eol = &clear_to_eol,
        .set_color = &set_color,
        .move_cursor = &move_cursor,
        .repeat_byte = &term_repeat_byte,
        .save_title = &no_op,
        .restore_title = &no_op,
        .set_title = &no_op_s,
        .control_codes = {
            .init = STRING_VIEW_INIT,
            .deinit = STRING_VIEW_INIT,
            .keypad_off = get_terminfo_string_view("rmkx"),
            .keypad_on = get_terminfo_string_view("smkx"),
            .cup_mode_off = get_terminfo_string_view("rmcup"),
            .cup_mode_on = get_terminfo_string_view("smcup"),
            .show_cursor = get_terminfo_string_view("cnorm"),
            .hide_cursor = get_terminfo_string_view("civis")
        }
    };

    if (xstreq(getenv("COLORTERM"), "truecolor")) {
        // Just use the built-in ecma48_set_color() function if true color
        // support is indicated. This bypasses tputs(3), but no true color
        // terminal in existence actually depends on archaic tputs(3)
        // features (like e.g. baudrate-dependant padding).
        terminal.color_type = TERM_TRUE_COLOR;
        terminal.set_color = &ecma48_set_color;
    } else {
        switch (tigetnum("colors")) {
        case 16777216:
            terminal.color_type = TERM_TRUE_COLOR;
            terminal.set_color = &ecma48_set_color;
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
        }
    }

    for (size_t i = 0; i < ARRAY_COUNT(keymap); i++) {
        const char *code = get_terminfo_string(keymap[i].code);
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
                terminal.parse_key_sequence = &parse_key_from_keymap;
            }
        }
    }

    return true; // Initialization succeeded
}

#else

bool term_init_terminfo(const char* UNUSED_ARG(term))
{
    return false; // terminfo not available
}

#endif // ifndef TERMINFO_DISABLE
