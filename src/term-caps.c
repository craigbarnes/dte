#include "term.h"

#ifdef TERMINFO_DISABLE

#include "common.h" // For str_has_prefix

int term_init(const char *const term)
{
    static const TerminalCapabilities xterm_caps = {
        .ut = false,
        .colors = 8,
        .strings = {
            [STR_CAP_CMD_ce] = "\033[K", // Clear to end of line
            [STR_CAP_CMD_ke] = "\033[?1l\033>", // Turn keypad off
            [STR_CAP_CMD_ks] = "\033[?1h\033=", // Turn keypad on
            [STR_CAP_CMD_te] = "\033[?1049l", // End program that uses cursor motion
            [STR_CAP_CMD_ti] = "\033[?1049h", // Begin program that uses cursor motion
            [STR_CAP_CMD_vi] = "\033[?25l", // Hide cursor

            // Terminfo actually returns 2 escape sequences for the "cnorm"
            // capability ("\033[?12l\033[?25h" for xterm and
            // "\033[?34h\033[?25h" for tmux/screen) but using just the
            // second sequence ("\033[?25h"), which is common to all 3
            // terminals, seems to work.
            [STR_CAP_CMD_ve] = "\033[?25h", // Show cursor
        },
        .keymap = {
            {KEY_INSERT, "\033[2~"},
            {KEY_DELETE, "\033[3~"},
            {KEY_HOME, "\033OH"},
            {KEY_END, "\033OF"},
            {KEY_PAGE_UP, "\033[5~"},
            {KEY_PAGE_DOWN, "\033[6~"},
            {KEY_LEFT, "\033OD"},
            {KEY_RIGHT, "\033OC"},
            {KEY_UP, "\033OA"},
            {KEY_DOWN, "\033OB"},
            {KEY_F1, "\033OP"},
            {KEY_F2, "\033OQ"},
            {KEY_F3, "\033OR"},
            {KEY_F4, "\033OS"},
            {KEY_F5, "\033[15~"},
            {KEY_F6, "\033[17~"},
            {KEY_F7, "\033[18~"},
            {KEY_F8, "\033[19~"},
            {KEY_F9, "\033[20~"},
            {KEY_F10, "\033[21~"},
            {KEY_F11, "\033[23~"},
            {KEY_F12, "\033[24~"},
        },
    };

    if (
        str_has_prefix(term, "tmux")
        || str_has_prefix(term, "xterm")
        || str_has_prefix(term, "screen")
    ) {
        term_cap = xterm_caps;
    }

    if (str_has_suffix(term, "256color")) {
        term_cap.colors = 256;
    }

    term_setup_extra_keys(term);
    return 0;
}

#else

#include "cursed.h"

static const char *const string_cap_map[NR_STR_CAPS] = {
    "acsc", // acs_chars,
    "rmacs", // exit_alt_charset_mode,
    "smacs", // enter_alt_charset_mode,
    "el", // clr_eol,
    "rmkx", // keypad_local,
    "smkx", // keypad_xmit,
    "rmcup", // exit_ca_mode,
    "smcup", // enter_ca_mode,
    "cnorm", // cursor_normal,
    "civis", // cursor_invisible,
};

static const char *const key_cap_map[NR_SPECIAL_KEYS] = {
    "kich1", // key_ic,
    "kdch1", // key_dc,
    "khome", // key_home,
    "kend", // key_end,
    "kpp", // key_ppage,
    "knp", // key_npage,
    "kcub1", // key_left,
    "kcuf1", // key_right,
    "kcuu1", // key_up,
    "kcud1", // key_down,
    "kf1", // key_f1,
    "kf2", // key_f2,
    "kf3", // key_f3,
    "kf4", // key_f4,
    "kf5", // key_f5,
    "kf6", // key_f6,
    "kf7", // key_f7,
    "kf8", // key_f8,
    "kf9", // key_f9,
    "kf10", // key_f10,
    "kf11", // key_f11,
    "kf12", // key_f12,
};

void term_read_caps(void)
{
    term_cap.ut = curses_bool_cap("bce"); // back_color_erase
    term_cap.colors = curses_int_cap("colors"); // max_colors
    for (size_t i = 0; i < NR_STR_CAPS; i++) {
        term_cap.strings[i] = curses_str_cap(string_cap_map[i]);
    }

    for (size_t i = 0; i < NR_SPECIAL_KEYS; i++) {
        term_cap.keymap[i].key = KEY_SPECIAL_MIN + i;
        term_cap.keymap[i].code = curses_str_cap(key_cap_map[i]);
    }

    term_cap.keymap[NR_SPECIAL_KEYS].key = MOD_SHIFT | KEY_LEFT;
    term_cap.keymap[NR_SPECIAL_KEYS].code = curses_str_cap("kLFT");

    term_cap.keymap[NR_SPECIAL_KEYS + 1].key = MOD_SHIFT | KEY_RIGHT;
    term_cap.keymap[NR_SPECIAL_KEYS + 1].code = curses_str_cap("kRIT");
}

int term_init(const char *term)
{
    int rc = curses_init(term);
    if (rc != 0) {
        return rc;
    }
    term_read_caps();
    term_setup_extra_keys(term);
    return 0;
}

#endif
