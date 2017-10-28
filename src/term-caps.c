#include "term.h"
#include "common.h" // For str_has_prefix

#ifndef TERMINFO_DISABLE

// These are normally declared in the <curses.h> and <term.h> headers.
// They are not included here because of the insane number of unprefixed
// symbols they declare and because of previous bugs caused by using them.
int setupterm(const char *term, int filedes, int *errret);
int tigetflag(const char *capname);
int tigetnum(const char *capname);
char *tigetstr(const char *capname);

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

static void term_read_caps(void)
{
    term_cap.back_color_erase = tigetflag("bce");
    term_cap.max_colors = tigetnum("colors");

    static_assert(ARRAY_COUNT(term_cap.strings) == 7);
    term_cap.strings[TERMCAP_CLEAR_TO_EOL] = curses_str_cap("el");
    term_cap.strings[TERMCAP_KEYPAD_OFF] = curses_str_cap("rmkx");
    term_cap.strings[TERMCAP_KEYPAD_ON] = curses_str_cap("smkx");
    term_cap.strings[TERMCAP_CUP_MODE_OFF] = curses_str_cap("rmcup");
    term_cap.strings[TERMCAP_CUP_MODE_ON] = curses_str_cap("smcup");
    term_cap.strings[TERMCAP_SHOW_CURSOR] = curses_str_cap("cnorm");
    term_cap.strings[TERMCAP_HIDE_CURSOR] = curses_str_cap("civis");

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

    static_assert(ARRAY_COUNT(term_cap.keymap) == ARRAY_COUNT(key_cap_map));
    for (size_t i = 0; i < ARRAY_COUNT(key_cap_map); i++) {
        term_cap.keymap[i].key = key_cap_map[i].key;
        term_cap.keymap[i].code = curses_str_cap(key_cap_map[i].capname);
    }
}

static void term_init_fallback(const char *const term)
{
    // Initialize ncurses (or call exit(3) on failure)
    setupterm(term, 1, (int*)0);
    term_read_caps();
}

#else

static const TerminalCapabilities ansi_caps = {
    .max_colors = 8,
    .strings = {
        [TERMCAP_CLEAR_TO_EOL] = "\033[K"
    },
    .keymap = {
        {KEY_LEFT, "\033[D"},
        {KEY_RIGHT, "\033[C"},
        {KEY_UP, "\033[A"},
        {KEY_DOWN, "\033[B"},
    }
};

static void term_init_fallback(const char *const UNUSED(term))
{
    term_cap = ansi_caps;
}

#endif // ifndef TERMINFO_DISABLE

static const TerminalCapabilities xterm_caps = {
    .back_color_erase = false,
    .max_colors = 8,
    .strings = {
        [TERMCAP_CLEAR_TO_EOL] = "\033[K",
        [TERMCAP_KEYPAD_OFF] = "\033[?1l\033>",
        [TERMCAP_KEYPAD_ON] = "\033[?1h\033=",
        [TERMCAP_CUP_MODE_OFF] = "\033[?1049l",
        [TERMCAP_CUP_MODE_ON] = "\033[?1049h",
        [TERMCAP_HIDE_CURSOR] = "\033[?25l",

        // Terminfo actually returns 2 escape sequences for the "cnorm"
        // capability ("\033[?12l\033[?25h" for xterm and
        // "\033[?34h\033[?25h" for tmux/screen) but using just the
        // second sequence ("\033[?25h"), which is common to all 3
        // terminals, seems to work.
        [TERMCAP_SHOW_CURSOR] = "\033[?25h",
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

void term_init(const char *const term)
{
    if (
        str_has_prefix(term, "tmux")
        || str_has_prefix(term, "xterm")
        || str_has_prefix(term, "screen")
    ) {
        term_cap = xterm_caps;
        if (str_has_suffix(term, "256color")) {
            term_cap.max_colors = 256;
        }
    } else {
        term_init_fallback(term);
    }

    term_setup_extra_keys(term);
}
