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

// See terminfo(5)
static void term_read_caps(void)
{
    terminal.can_bg_color_erase = tigetflag("bce");
    terminal.max_colors = tigetnum("colors");
    terminal.width = tigetnum("cols");
    terminal.height = tigetnum("lines");

    terminal.control_codes = (TermControlCodes) {
        .clear_to_eol = curses_str_cap("el"),
        .keypad_off = curses_str_cap("rmkx"),
        .keypad_on = curses_str_cap("smkx"),
        .cup_mode_off = curses_str_cap("rmcup"),
        .cup_mode_on = curses_str_cap("smcup"),
        .show_cursor = curses_str_cap("cnorm"),
        .hide_cursor = curses_str_cap("civis")
    };

    static const struct {
        const Key key;
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

    static const size_t keymap_length = ARRAY_COUNT(key_cap_map);
    TermKeyMap *keymap = xnew(TermKeyMap, keymap_length);

    for (size_t i = 0; i < keymap_length; i++) {
        keymap[i].key = key_cap_map[i].key;
        keymap[i].code = curses_str_cap(key_cap_map[i].capname);
    }

    terminal.keymap = keymap;
    terminal.keymap_length = keymap_length;
}

static void term_init_fallback(const char *const term)
{
    // Initialize ncurses (or call exit(3) on failure)
    setupterm(term, 1, (int*)0);
    term_read_caps();
}

#else

static const TermKeyMap ansi_keymap[] = {
    {KEY_LEFT, "\033[D"},
    {KEY_RIGHT, "\033[C"},
    {KEY_UP, "\033[A"},
    {KEY_DOWN, "\033[B"},
};

static const TerminalInfo terminal_ansi = {
    .max_colors = 8,
    .width = 80,
    .height = 24,
    .keymap = ansi_keymap,
    .keymap_length = ARRAY_COUNT(ansi_keymap),
    .control_codes = {
        .clear_to_eol = "\033[K"
    }
};

static void term_init_fallback(const char *const UNUSED(term))
{
    terminal = terminal_ansi;
}

#endif // ifndef TERMINFO_DISABLE

static const TermKeyMap xterm_keymap[] = {
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
};

static const TerminalInfo terminal_xterm = {
    .can_bg_color_erase = false,
    .max_colors = 8,
    .width = 80,
    .height = 24,
    .keymap = xterm_keymap,
    .keymap_length = ARRAY_COUNT(xterm_keymap),
    .control_codes = {
        .clear_to_eol = "\033[K",
        .keypad_off = "\033[?1l\033>",
        .keypad_on = "\033[?1h\033=",
        .cup_mode_off = "\033[?1049l",
        .cup_mode_on = "\033[?1049h",
        .hide_cursor = "\033[?25l",

        // Terminfo actually returns 2 escape sequences for the "cnorm"
        // capability ("\033[?12l\033[?25h" for xterm and
        // "\033[?34h\033[?25h" for tmux/screen) but using just the
        // second sequence ("\033[?25h"), which is common to all 3
        // terminals, seems to work.
        .show_cursor = "\033[?25h",
    }
};

void term_init(const char *const term)
{
    if (
        str_has_prefix(term, "tmux")
        || str_has_prefix(term, "xterm")
        || str_has_prefix(term, "screen")
    ) {
        terminal = terminal_xterm;
        if (str_has_suffix(term, "256color")) {
            terminal.max_colors = 256;
        }
    } else {
        term_init_fallback(term);
    }

    term_setup_extra_keys(term);
}
