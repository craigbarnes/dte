#include "term.h"
#include "common.h" // For str_has_prefix

#define ANSI_ATTRS (ATTR_UNDERLINE | ATTR_REVERSE | ATTR_BLINK | ATTR_BOLD)

#ifndef TERMINFO_DISABLE

// These are normally declared in the <curses.h> and <term.h> headers.
// They are not included here because of the insane number of unprefixed
// symbols they declare and because of previous bugs caused by using them.
int setupterm(const char *term, int filedes, int *errret);
int tigetflag(const char *capname);
int tigetnum(const char *capname);
char *tigetstr(const char *capname);

static char *curses_str_cap(const char *const name)
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
    terminal.back_color_erase = tigetflag("bce");
    terminal.max_colors = tigetnum("colors");
    terminal.width = tigetnum("cols");
    terminal.height = tigetnum("lines");

    terminal.attributes =
        (xstreq(curses_str_cap("smul"), "\033[4m") ? ATTR_UNDERLINE : 0)
        + (xstreq(curses_str_cap("rev"), "\033[7m") ? ATTR_REVERSE : 0)
        + (xstreq(curses_str_cap("blink"), "\033[5m") ? ATTR_BLINK : 0)
        + (xstreq(curses_str_cap("dim"), "\033[2m") ? ATTR_DIM : 0)
        + (xstreq(curses_str_cap("bold"), "\033[1m") ? ATTR_BOLD : 0)
        + (xstreq(curses_str_cap("invis"), "\033[8m") ? ATTR_INVIS : 0)
        + (xstreq(curses_str_cap("sitm"), "\033[3m") ? ATTR_ITALIC : 0)
    ;

    const int ncv = tigetnum("ncv");
    if (ncv <= 0) {
        terminal.ncv_attributes = 0;
    } else {
        terminal.ncv_attributes = (unsigned short)ncv;
        // The ATTR_* bitflag values used in this codebase are mostly
        // the same as those used by terminfo, with the exception of
        // ITALIC, which is bit 7 here, but bit 15 in terminfo. It
        // must therefore be manually converted.
        if ((ncv & (1 << 15)) && !(ncv & ATTR_ITALIC)) {
            terminal.ncv_attributes |= ATTR_ITALIC;
        }
    }

    static TermControlCodes tcc;
    tcc = (TermControlCodes) {
        .clear_to_eol = curses_str_cap("el"),
        .keypad_off = curses_str_cap("rmkx"),
        .keypad_on = curses_str_cap("smkx"),
        .cup_mode_off = curses_str_cap("rmcup"),
        .cup_mode_on = curses_str_cap("smcup"),
        .show_cursor = curses_str_cap("cnorm"),
        .hide_cursor = curses_str_cap("civis")
    };
    terminal.control_codes = &tcc;

    static TermKeyMap keymap[] = {
        {"kich1", KEY_INSERT},
        {"kdch1", KEY_DELETE},
        {"khome", KEY_HOME},
        {"kend", KEY_END},
        {"kpp", KEY_PAGE_UP},
        {"knp", KEY_PAGE_DOWN},
        {"kcub1", KEY_LEFT},
        {"kcuf1", KEY_RIGHT},
        {"kcuu1", KEY_UP},
        {"kcud1", KEY_DOWN},
        {"kf1", KEY_F1},
        {"kf2", KEY_F2},
        {"kf3", KEY_F3},
        {"kf4", KEY_F4},
        {"kf5", KEY_F5},
        {"kf6", KEY_F6},
        {"kf7", KEY_F7},
        {"kf8", KEY_F8},
        {"kf9", KEY_F9},
        {"kf10", KEY_F10},
        {"kf11", KEY_F11},
        {"kf12", KEY_F12},
        {"kLFT", MOD_SHIFT | KEY_LEFT},
        {"kRIT", MOD_SHIFT | KEY_RIGHT},
        {"kHOM", MOD_SHIFT | KEY_HOME},
        {"kEND", MOD_SHIFT | KEY_END},
        {"kDC", MOD_SHIFT | KEY_DELETE},

        // Extended capabilities (see ncurses user_caps(5) manpage)
        {"kDC5", MOD_CTRL | KEY_DELETE},
        {"kDC6", MOD_CTRL | MOD_SHIFT | KEY_DELETE},
        {"kUP", MOD_SHIFT | KEY_UP},
        {"kDN", MOD_SHIFT | KEY_DOWN},
        {"kLFT3", MOD_META | KEY_LEFT},
        {"kRIT3", MOD_META | KEY_RIGHT},
        {"kUP3", MOD_META | KEY_UP},
        {"kDN3", MOD_META | KEY_DOWN},
        {"kLFT5", MOD_CTRL | KEY_LEFT},
        {"kRIT5", MOD_CTRL | KEY_RIGHT},
        {"kUP5", MOD_CTRL | KEY_UP},
        {"kDN5", MOD_CTRL | KEY_DOWN},
        {"kLFT4", MOD_META | MOD_SHIFT | KEY_LEFT},
        {"kRIT4", MOD_META | MOD_SHIFT | KEY_RIGHT},
        {"kUP4", MOD_META | MOD_SHIFT | KEY_UP},
        {"kDN4", MOD_META | MOD_SHIFT | KEY_DOWN},
        {"kLFT6", MOD_CTRL | MOD_SHIFT | KEY_LEFT},
        {"kRIT6", MOD_CTRL | MOD_SHIFT | KEY_RIGHT},
        {"kUP6", MOD_CTRL | MOD_SHIFT | KEY_UP},
        {"kDN6", MOD_CTRL | MOD_SHIFT | KEY_DOWN},
        {"kLFT7", MOD_CTRL | MOD_META | KEY_LEFT},
        {"kRIT7", MOD_CTRL | MOD_META | KEY_RIGHT},
        {"kUP7", MOD_CTRL | MOD_META | KEY_UP},
        {"kDN7", MOD_CTRL | MOD_META | KEY_DOWN},
        {"kLFT8", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_LEFT},
        {"kRIT8", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_RIGHT},
        {"kUP8", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_UP},
        {"kDN8", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_DOWN},
    };

    static const size_t keymap_length = ARRAY_COUNT(keymap);

    // Replace all capability names in the keymap array above with the
    // corresponding escape sequences fetched from terminfo.
    for (size_t i = 0; i < keymap_length; i++) {
        const char *const code = curses_str_cap(keymap[i].code);
        keymap[i].code = code;
    }

    terminal.keymap = keymap;
    terminal.keymap_length = keymap_length;
}

static void term_init_fallback(const char *const term)
{
    // Initialize terminfo database (or call exit(3) on failure)
    setupterm(term, 1, (int*)0);

    term_read_caps();
}

#else

static const TermKeyMap ansi_keymap[] = {
    {"\033[A", KEY_UP},
    {"\033[B", KEY_DOWN},
    {"\033[C", KEY_RIGHT},
    {"\033[D", KEY_LEFT},
};

static const TermControlCodes ansi_control_codes = {
    .clear_to_eol = "\033[K"
};

static const TerminalInfo terminal_ansi = {
    .max_colors = 8,
    .width = 80,
    .height = 24,
    .attributes = ANSI_ATTRS,
    .ncv_attributes = ATTR_UNDERLINE,
    .keymap = ansi_keymap,
    .keymap_length = ARRAY_COUNT(ansi_keymap),
    .control_codes = &ansi_control_codes
};

static void term_init_fallback(const char *const UNUSED(term))
{
    terminal = terminal_ansi;
}

#endif // ifndef TERMINFO_DISABLE

#define XTERM_KEYMAP_COMMON \
    {"\033[2~", KEY_INSERT}, \
    {"\033[3~", KEY_DELETE}, \
    {"\033OH", KEY_HOME}, \
    {"\033[1~", KEY_HOME}, \
    {"\033OF", KEY_END}, \
    {"\033[4~", KEY_END}, \
    {"\033[5~", KEY_PAGE_UP}, \
    {"\033[6~", KEY_PAGE_DOWN}, \
    {"\033OD", KEY_LEFT}, \
    {"\033OC", KEY_RIGHT}, \
    {"\033OA", KEY_UP}, \
    {"\033OB", KEY_DOWN}, \
    {"\033OP", KEY_F1}, \
    {"\033OQ", KEY_F2}, \
    {"\033OR", KEY_F3}, \
    {"\033OS", KEY_F4}, \
    {"\033[15~", KEY_F5}, \
    {"\033[17~", KEY_F6}, \
    {"\033[18~", KEY_F7}, \
    {"\033[19~", KEY_F8}, \
    {"\033[20~", KEY_F9}, \
    {"\033[21~", KEY_F10}, \
    {"\033[23~", KEY_F11}, \
    {"\033[24~", KEY_F12}, \
    {"\033[1;2A", MOD_SHIFT | KEY_UP}, \
    {"\033[1;2B", MOD_SHIFT | KEY_DOWN}, \
    {"\033[1;2D", MOD_SHIFT | KEY_LEFT}, \
    {"\033[1;2C", MOD_SHIFT | KEY_RIGHT}, \
    {"\033[1;5D", MOD_CTRL | KEY_LEFT}, \
    {"\033[1;5C", MOD_CTRL | KEY_RIGHT}, \
    {"\033[1;5A", MOD_CTRL | KEY_UP}, \
    {"\033[1;5B", MOD_CTRL | KEY_DOWN}, \
    {"\033[1;3D", MOD_META | KEY_LEFT}, \
    {"\033[1;3C", MOD_META | KEY_RIGHT}, \
    {"\033[1;3A", MOD_META | KEY_UP}, \
    {"\033[1;3B", MOD_META | KEY_DOWN}, \
    {"\033[5;2~", MOD_SHIFT | KEY_PAGE_UP}, \
    {"\033[6;2~", MOD_SHIFT | KEY_PAGE_DOWN}, \
    {"\033[5;5~", MOD_CTRL | KEY_PAGE_UP}, \
    {"\033[6;5~", MOD_CTRL | KEY_PAGE_DOWN}, \
    /* Fix keypad when numlock is off */ \
    {"\033Oo", '/'}, \
    {"\033Oj", '*'}, \
    {"\033Om", '-'}, \
    {"\033Ok", '+'}, \
    {"\033OM", '\r'},

static const TermKeyMap st_keymap[] = {
    XTERM_KEYMAP_COMMON
};

static const TermKeyMap xterm_keymap[] = {
    XTERM_KEYMAP_COMMON
    {"\033[1;6D", MOD_CTRL | MOD_SHIFT | KEY_LEFT},
    {"\033[1;6C", MOD_CTRL | MOD_SHIFT | KEY_RIGHT},
    {"\033[1;6A", MOD_CTRL | MOD_SHIFT | KEY_UP},
    {"\033[1;6B", MOD_CTRL | MOD_SHIFT | KEY_DOWN},
    {"\033[1;4D", MOD_META | MOD_SHIFT | KEY_LEFT},
    {"\033[1;4C", MOD_META | MOD_SHIFT | KEY_RIGHT},
    {"\033[1;4A", MOD_META | MOD_SHIFT | KEY_UP},
    {"\033[1;4B", MOD_META | MOD_SHIFT | KEY_DOWN},
    {"\033[1;7D", MOD_CTRL | MOD_META | KEY_LEFT},
    {"\033[1;7C", MOD_CTRL | MOD_META | KEY_RIGHT},
    {"\033[1;7A", MOD_CTRL | MOD_META | KEY_UP},
    {"\033[1;7B", MOD_CTRL | MOD_META | KEY_DOWN},
    {"\033[1;8D", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_LEFT},
    {"\033[1;8C", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_RIGHT},
    {"\033[1;8A", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_UP},
    {"\033[1;8B", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_DOWN},
    {"\033[5;3~", MOD_META | KEY_PAGE_UP},
    {"\033[6;3~", MOD_META | KEY_PAGE_DOWN},
    {"\033[5;4~", MOD_META | MOD_SHIFT | KEY_PAGE_UP},
    {"\033[6;4~", MOD_META | MOD_SHIFT | KEY_PAGE_DOWN},
    {"\033[5;6~", MOD_CTRL | MOD_SHIFT | KEY_PAGE_UP},
    {"\033[6;6~", MOD_CTRL | MOD_SHIFT | KEY_PAGE_DOWN},
    {"\033[5;7~", MOD_CTRL | MOD_META | KEY_PAGE_UP},
    {"\033[6;7~", MOD_CTRL | MOD_META | KEY_PAGE_DOWN},
    {"\033[5;8~", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_PAGE_UP},
    {"\033[6;8~", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_PAGE_DOWN},
    {"\033[1;2H", MOD_SHIFT | KEY_HOME},
    {"\033[1;2F", MOD_SHIFT | KEY_END},
    {"\033[3;5~", MOD_CTRL | KEY_DELETE},
    {"\033[3;2~", MOD_SHIFT | KEY_DELETE},
    {"\033[3;6~", MOD_CTRL | MOD_SHIFT | KEY_DELETE},
    {"\033[Z", MOD_SHIFT | '\t'},
};

static const TermKeyMap rxvt_keymap[] = {
    {"\033[2~", KEY_INSERT},
    {"\033[3~", KEY_DELETE},
    {"\033[7~", KEY_HOME},
    {"\033[8~", KEY_END},
    {"\033[5~", KEY_PAGE_UP},
    {"\033[6~", KEY_PAGE_DOWN},
    {"\033[D", KEY_LEFT},
    {"\033[C", KEY_RIGHT},
    {"\033[A", KEY_UP},
    {"\033[B", KEY_DOWN},
    {"\033[11~", KEY_F1},
    {"\033[12~", KEY_F2},
    {"\033[13~", KEY_F3},
    {"\033[14~", KEY_F4},
    {"\033[15~", KEY_F5},
    {"\033[17~", KEY_F6},
    {"\033[18~", KEY_F7},
    {"\033[19~", KEY_F8},
    {"\033[20~", KEY_F9},
    {"\033[21~", KEY_F10},
    {"\033[23~", KEY_F11},
    {"\033[24~", KEY_F12},
    {"\033[7$", MOD_SHIFT | KEY_HOME},
    {"\033[8$", MOD_SHIFT | KEY_END},
    {"\033[d", MOD_SHIFT | KEY_LEFT},
    {"\033[c", MOD_SHIFT | KEY_RIGHT},
    {"\033[a", MOD_SHIFT | KEY_UP},
    {"\033[b", MOD_SHIFT | KEY_DOWN},
    {"\033Od", MOD_CTRL | KEY_LEFT},
    {"\033Oc", MOD_CTRL | KEY_RIGHT},
    {"\033Oa", MOD_CTRL | KEY_UP},
    {"\033Ob", MOD_CTRL | KEY_DOWN},
    {"\033\033[D", MOD_META | KEY_LEFT},
    {"\033\033[C", MOD_META | KEY_RIGHT},
    {"\033\033[A", MOD_META | KEY_UP},
    {"\033\033[B", MOD_META | KEY_DOWN},
    {"\033\033[d", MOD_META | MOD_SHIFT | KEY_LEFT},
    {"\033\033[c", MOD_META | MOD_SHIFT | KEY_RIGHT},
    {"\033\033[a", MOD_META | MOD_SHIFT | KEY_UP},
    {"\033\033[b", MOD_META | MOD_SHIFT | KEY_DOWN},
};

static const TermControlCodes xterm_control_codes = {
    .clear_to_eol = "\033[K",
    .keypad_off = "\033[?1l\033>",
    .keypad_on = "\033[?1h\033=",
    .cup_mode_off = "\033[?1049l",
    .cup_mode_on = "\033[?1049h",
    .hide_cursor = "\033[?25l",
    .show_cursor = "\033[?25h",
    .save_title = "\033[22;2t",
    .restore_title = "\033[23;2t",
    .set_title_begin = "\033]2;",
    .set_title_end = "\007",
};

static const TermControlCodes rxvt_control_codes = {
    .clear_to_eol = "\033[K",
    .keypad_off = "\033>",
    .keypad_on = "\033=",
    .cup_mode_off = "\033[2J\033[?47l\0338",
    .cup_mode_on = "\0337\033[?47h",
    .hide_cursor = "\033[?25l",
    .show_cursor = "\033[?25h",
    .save_title = "\033[22;2t",
    .restore_title = "\033[23;2t",
    .set_title_begin = "\033]2;",
    .set_title_end = "\007",
};

static const TerminalInfo terminal_xterm = {
    .back_color_erase = false,
    .max_colors = 8,
    .width = 80,
    .height = 24,
    .attributes = ANSI_ATTRS | ATTR_INVIS | ATTR_DIM | ATTR_ITALIC,
    .keymap = xterm_keymap,
    .keymap_length = ARRAY_COUNT(xterm_keymap),
    .control_codes = &xterm_control_codes
};

static const TerminalInfo terminal_st = {
    .back_color_erase = true,
    .max_colors = 8,
    .width = 80,
    .height = 24,
    .attributes = ANSI_ATTRS | ATTR_INVIS | ATTR_DIM | ATTR_ITALIC,
    .keymap = st_keymap,
    .keymap_length = ARRAY_COUNT(st_keymap),
    .control_codes = &xterm_control_codes
};

static const TerminalInfo terminal_rxvt = {
    .back_color_erase = true,
    .max_colors = 8,
    .width = 80,
    .height = 24,
    .attributes = ANSI_ATTRS,
    .keymap = rxvt_keymap,
    .keymap_length = ARRAY_COUNT(rxvt_keymap),
    .control_codes = &rxvt_control_codes
};

static bool term_match(const char *term, const char *prefix)
{
    if (str_has_prefix(term, prefix) == false) {
        return false;
    }
    switch (term[strlen(prefix)]) {
    // Exact match
    case '\0':
    // Prefix match
    case '-':
    case '+':
        return true;
    }
    return false;
}

void term_init(const char *const term)
{
    if (getenv("DTE_FORCE_TERMINFO")) {
#ifndef TERMINFO_DISABLE
        term_init_fallback(term);
        return;
#else
        fputs("'DTE_FORCE_TERMINFO' set but terminfo not linked\n", stderr);
        exit(1);
#endif
    }

    if (
        term_match(term, "tmux")
        || term_match(term, "xterm")
        || term_match(term, "screen")
    ) {
        terminal = terminal_xterm;
    } else if (term_match(term, "st") || term_match(term, "stterm")) {
        terminal = terminal_st;
    } else if (term_match(term, "rxvt")) {
        terminal = terminal_rxvt;
    } else {
        term_init_fallback(term);
    }

    if (str_has_suffix(term, "256color")) {
        terminal.max_colors = 256;
    }
}
