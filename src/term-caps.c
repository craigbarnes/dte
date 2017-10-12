#include "term.h"

#ifdef TERMINFO_DISABLE

#include "common.h" // For str_has_prefix

void term_init(const char *const term)
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
}

#else

// These are normally declared in the <curses.h> and <term.h> headers.
// They are not included here because of the insane number of unprefixed
// symbols they declare and because of previous bugs caused by using them.
int setupterm(const char *term, int filedes, int *errret);
int tigetflag(const char *capname);
int tigetnum(const char *capname);
char *tigetstr(const char *capname);

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

static const struct {
    Key key;
    const char *const capname;
} key_cap_map[] = {
    {KEY_INSERT, "kich1"},
    {KEY_DELETE, "kdch1"},
    {KEY_HOME, "khome"},
    {KEY_END, "kend"},
    {KEY_PAGE_UP, "kpp"},
    {KEY_PAGE_DOWN, "knp"},
    {KEY_LEFT, "kcub1"},
    {KEY_RIGHT, "kcuf1"},
    {KEY_UP, "kcuu1"},
    {KEY_DOWN, "kcud1"},
    {KEY_F1, "kf1"},
    {KEY_F2, "kf2"},
    {KEY_F3, "kf3"},
    {KEY_F4, "kf4"},
    {KEY_F5, "kf5"},
    {KEY_F6, "kf6"},
    {KEY_F7, "kf7"},
    {KEY_F8, "kf8"},
    {KEY_F9, "kf9"},
    {KEY_F10, "kf10"},
    {KEY_F11, "kf11"},
    {KEY_F12, "kf12"},

    {MOD_SHIFT | KEY_LEFT, "kLFT"},
    {MOD_SHIFT | KEY_RIGHT, "kRIT"},
    {MOD_SHIFT | KEY_HOME, "kHOM"},
    {MOD_SHIFT | KEY_END, "kEND"},
};

static int curses_bool_cap(const char *name)
{
    return tigetflag(name);
}

static int curses_int_cap(const char *name)
{
    return tigetnum(name);
}

static char *curses_str_cap(const char *name)
{
    char *str = tigetstr(name);

    if (str == (char *)-1) {
        // Not a string cap (bug?)
        return NULL;
    }
    // NULL = canceled or absent
    return str;
}

void term_read_caps(void)
{
    term_cap.ut = curses_bool_cap("bce"); // back_color_erase
    term_cap.colors = curses_int_cap("colors"); // max_colors

    static_assert(ARRAY_COUNT(term_cap.strings) == ARRAY_COUNT(string_cap_map));
    for (size_t i = 0; i < NR_STR_CAPS; i++) {
        term_cap.strings[i] = curses_str_cap(string_cap_map[i]);
    }

    static_assert(ARRAY_COUNT(term_cap.keymap) == ARRAY_COUNT(key_cap_map));
    for (size_t i = 0; i < ARRAY_COUNT(key_cap_map); i++) {
        term_cap.keymap[i].key = key_cap_map[i].key;
        term_cap.keymap[i].code = curses_str_cap(key_cap_map[i].capname);
    }
}

void term_init(const char *const term)
{
    // Initialize ncurses (or call exit(3) on failure)
    setupterm(term, 1, (int*)0);

    term_read_caps();
    term_setup_extra_keys(term);
}

#endif
