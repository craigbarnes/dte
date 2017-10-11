#include "term.h"
#include "common.h"
#include "editor.h"
#include "cursed.h"

#undef CTRL

#include <sys/ioctl.h>
#include <termios.h>

typedef struct {
    int key;
    const char *const code;
    unsigned int terms;
} KeyMap;

enum {
    T_RXVT = 1,
    T_SCREEN = 2,
    T_ST = 4,
    T_TMUX = 8,
    T_XTERM = 16,
    T_XTERM_LIKE = T_XTERM | T_SCREEN | T_TMUX,
    T_ALL = T_XTERM_LIKE | T_RXVT | T_ST,
};

// Prefixes; st-256color matches st
static const char *const terms[] = {
    "rxvt",
    "screen",
    "st",
    "tmux",
    "xterm",
};

static const KeyMap builtin_keys[] = {
    // ansi
    {KEY_LEFT, "\033[D", T_ALL},
    {KEY_RIGHT, "\033[C", T_ALL},
    {KEY_UP, "\033[A", T_ALL},
    {KEY_DOWN, "\033[B", T_ALL},

    // ???
    {KEY_HOME, "\033[1~", T_ALL},
    {KEY_END, "\033[4~", T_ALL},

    // Fix keypad when numlock is off
    {'/', "\033Oo", T_ALL},
    {'*', "\033Oj", T_ALL},
    {'-', "\033Om", T_ALL},
    {'+', "\033Ok", T_ALL},
    {'\r', "\033OM", T_ALL},

    {MOD_SHIFT | KEY_LEFT, "\033[d", T_RXVT},
    {MOD_SHIFT | KEY_RIGHT,"\033[c", T_RXVT},
    {MOD_SHIFT | KEY_UP, "\033[a", T_RXVT},
    {MOD_SHIFT | KEY_DOWN, "\033[b", T_RXVT},
    {MOD_SHIFT | KEY_UP, "\033[1;2A", T_ST | T_XTERM_LIKE},
    {MOD_SHIFT | KEY_DOWN, "\033[1;2B", T_ST | T_XTERM_LIKE},
    {MOD_SHIFT | KEY_LEFT, "\033[1;2D", T_ST | T_XTERM_LIKE},
    {MOD_SHIFT | KEY_RIGHT,"\033[1;2C", T_ST | T_XTERM_LIKE},

    {MOD_CTRL | KEY_LEFT,  "\033Od", T_RXVT},
    {MOD_CTRL | KEY_RIGHT, "\033Oc", T_RXVT},
    {MOD_CTRL | KEY_UP,    "\033Oa", T_RXVT},
    {MOD_CTRL | KEY_DOWN,  "\033Ob", T_RXVT},
    {MOD_CTRL | KEY_LEFT,  "\033[1;5D", T_ST | T_XTERM_LIKE},
    {MOD_CTRL | KEY_RIGHT, "\033[1;5C", T_ST | T_XTERM_LIKE},
    {MOD_CTRL | KEY_UP,    "\033[1;5A", T_ST | T_XTERM_LIKE},
    {MOD_CTRL | KEY_DOWN,  "\033[1;5B", T_ST | T_XTERM_LIKE},

    {MOD_CTRL | MOD_SHIFT | KEY_LEFT,  "\033[1;6D", T_XTERM_LIKE},
    {MOD_CTRL | MOD_SHIFT | KEY_RIGHT, "\033[1;6C", T_XTERM_LIKE},
    {MOD_CTRL | MOD_SHIFT | KEY_UP,    "\033[1;6A", T_XTERM_LIKE},
    {MOD_CTRL | MOD_SHIFT | KEY_DOWN,  "\033[1;6B", T_XTERM_LIKE},

    {MOD_META | KEY_LEFT,  "\033\033[D", T_RXVT},
    {MOD_META | KEY_RIGHT, "\033\033[C", T_RXVT},
    {MOD_META | KEY_UP,    "\033\033[A", T_RXVT},
    {MOD_META | KEY_DOWN,  "\033\033[B", T_RXVT},
    {MOD_META | KEY_LEFT,  "\033[1;3D", T_ST | T_XTERM_LIKE},
    {MOD_META | KEY_RIGHT, "\033[1;3C", T_ST | T_XTERM_LIKE},
    {MOD_META | KEY_UP,    "\033[1;3A", T_ST | T_XTERM_LIKE},
    {MOD_META | KEY_DOWN,  "\033[1;3B", T_ST | T_XTERM_LIKE},

    {MOD_META | MOD_SHIFT | KEY_LEFT,  "\033\033[d", T_RXVT},
    {MOD_META | MOD_SHIFT | KEY_RIGHT, "\033\033[c", T_RXVT},
    {MOD_META | MOD_SHIFT | KEY_UP,    "\033\033[a", T_RXVT},
    {MOD_META | MOD_SHIFT | KEY_DOWN,  "\033\033[b", T_RXVT},
    {MOD_META | MOD_SHIFT | KEY_LEFT,  "\033[1;4D", T_XTERM_LIKE},
    {MOD_META | MOD_SHIFT | KEY_RIGHT, "\033[1;4C", T_XTERM_LIKE},
    {MOD_META | MOD_SHIFT | KEY_UP,    "\033[1;4A", T_XTERM_LIKE},
    {MOD_META | MOD_SHIFT | KEY_DOWN,  "\033[1;4B", T_XTERM_LIKE},

    {MOD_CTRL | MOD_META | MOD_SHIFT | KEY_LEFT,  "\033[1;8D", T_XTERM_LIKE},
    {MOD_CTRL | MOD_META | MOD_SHIFT | KEY_RIGHT, "\033[1;8C", T_XTERM_LIKE},
    {MOD_CTRL | MOD_META | MOD_SHIFT | KEY_UP,    "\033[1;8A", T_XTERM_LIKE},
    {MOD_CTRL | MOD_META | MOD_SHIFT | KEY_DOWN,  "\033[1;8B", T_XTERM_LIKE},

    {MOD_CTRL | KEY_DELETE, "\033[3;5~", T_XTERM_LIKE},
    {MOD_SHIFT | KEY_DELETE, "\033[3;2~", T_XTERM_LIKE},
    {MOD_CTRL | MOD_SHIFT | KEY_DELETE, "\033[3;6~", T_XTERM_LIKE},

    {MOD_SHIFT | '\t', "\033[Z", T_ST | T_XTERM_LIKE},
};

// These basic default values are for ANSI compatible terminals.
// They will be overwritten by term_init() in most cases.
struct term_cap term_cap = {
    .colors = 8,
    .strings = {
        [STR_CAP_CMD_ce] = "\033[K"
    },
    .keymap = {
        {KEY_LEFT, "\033[D"},
        {KEY_RIGHT, "\033[C"},
        {KEY_UP, "\033[A"},
        {KEY_DOWN, "\033[B"},
    }
};

static struct termios termios_save;
static char buffer[64];
static int buffer_pos;
static unsigned int term_flags; // T_*

static void buffer_num(unsigned int num)
{
    char stack[32];
    int pos = 0;

    do {
        stack[pos++] = (num % 10) + '0';
        num /= 10;
    } while (num);
    do {
        buffer[buffer_pos++] = stack[--pos];
    } while (pos);
}

static char *escape_key(const char *key, int len)
{
    static char buf[1024];
    int i, j = 0;

    for (i = 0; i < len && i < sizeof(buf) - 1; i++) {
        unsigned char ch = key[i];

        if (ch < 0x20) {
            buf[j++] = '^';
            ch |= 0x40;
        } else if (ch == 0x7f) {
            buf[j++] = '^';
            ch = '?';
        }
        buf[j++] = ch;
    }
    buf[j] = 0;
    return buf;
}

void term_setup_extra_keys(const char *const term)
{
    for (size_t i = 0; i < ARRAY_COUNT(terms); i++) {
        if (str_has_prefix(term, terms[i])) {
            term_flags = 1 << i;
            break;
        }
    }
}

void term_raw(void)
{
    // See termios(3)
    struct termios termios;

    tcgetattr(0, &termios);
    termios_save = termios;

    // Disable buffering
    // Disable echo
    // Disable generation of signals (free some control keys)
    termios.c_lflag &= ~(ICANON | ECHO | ISIG);

    // Disable CR to NL conversion (differentiate ^J from enter)
    // Disable flow control (free ^Q and ^S)
    termios.c_iflag &= ~(ICRNL | IXON | IXOFF);

    // Read at least 1 char on each read()
    termios.c_cc[VMIN] = 1;

    // Read blocks until there are MIN(VMIN, requested) bytes available
    termios.c_cc[VTIME] = 0;

    tcsetattr(0, 0, &termios);
}

void term_cooked(void)
{
    tcsetattr(0, 0, &termios_save);
}

static char input_buf[256];
static int input_buf_fill;
static bool input_can_be_truncated;

static void consume_input(int len)
{
    input_buf_fill -= len;
    if (input_buf_fill) {
        memmove(input_buf, input_buf + len, input_buf_fill);

        // Keys are sent faster than we can read
        input_can_be_truncated = true;
    }
}

static bool fill_buffer(void)
{
    int rc;

    if (input_buf_fill == sizeof(input_buf)) {
        return false;
    }

    if (!input_buf_fill) {
        input_can_be_truncated = false;
    }

    rc = read(0, input_buf + input_buf_fill, sizeof(input_buf) - input_buf_fill);
    if (rc <= 0) {
        return false;
    }
    input_buf_fill += rc;
    return true;
}

static bool fill_buffer_timeout(void)
{
    struct timeval tv = {
        .tv_sec = editor.options.esc_timeout / 1000,
        .tv_usec = (editor.options.esc_timeout % 1000) * 1000
    };
    fd_set set;
    int rc;

    FD_ZERO(&set);
    FD_SET(0, &set);
    rc = select(1, &set, NULL, NULL, &tv);
    if (rc > 0 && fill_buffer()) {
        return true;
    }
    return false;
}

static bool input_get_byte(unsigned char *ch)
{
    if (!input_buf_fill && !fill_buffer()) {
        return false;
    }
    *ch = input_buf[0];
    consume_input(1);
    return true;
}

static const KeyMap *find_key(bool *possibly_truncated)
{
    for (int i = 0; i < ARRAY_COUNT(builtin_keys); i++) {
        const KeyMap *entry = &builtin_keys[i];
        int len;

        if ((entry->terms & term_flags) == 0) {
            continue;
        }
        len = strlen(entry->code);
        if (len > input_buf_fill) {
            if (!memcmp(entry->code, input_buf, input_buf_fill)) {
                *possibly_truncated = true;
            }
        } else if (strncmp(entry->code, input_buf, len) == 0) {
            return entry;
        }
    }
    return NULL;
}

static bool read_special(int *key)
{
    const KeyMap *entry;
    bool possibly_truncated = false;

    if (DEBUG > 2) {
        d_print("keycode: '%s'\n", escape_key(input_buf, input_buf_fill));
    }
    for (size_t i = 0; i < ARRAY_COUNT(term_cap.keymap); i++) {
        const struct term_keymap *km = &term_cap.keymap[i];
        const char *keycode = km->code;
        int len;

        if (!keycode) {
            continue;
        }

        len = strlen(keycode);
        if (len > input_buf_fill) {
            // This might be a truncated escape sequence
            if (!memcmp(keycode, input_buf, input_buf_fill)) {
                possibly_truncated = true;
            }
            continue;
        }
        if (strncmp(keycode, input_buf, len)) {
            continue;
        }
        *key = km->key;
        consume_input(len);
        return true;
    }
    entry = find_key(&possibly_truncated);
    if (entry != NULL) {
        *key = entry->key;
        consume_input(strlen(entry->code));
        return true;
    }

    if (possibly_truncated && input_can_be_truncated && fill_buffer()) {
        return read_special(key);
    }
    return false;
}

static bool read_simple(int *key)
{
    unsigned char ch = 0;

    // > 0 bytes in buf
    input_get_byte(&ch);

    // Normal key
    if (editor.term_utf8 && ch > 0x7f) {
        /*
         * 10xx xxxx invalid
         * 110x xxxx valid
         * 1110 xxxx valid
         * 1111 0xxx valid
         * 1111 1xxx invalid
         */
        unsigned int u, bit = 1 << 6;
        int count = 0;

        while (ch & bit) {
            bit >>= 1;
            count++;
        }
        if (count == 0 || count > 3) {
            // Invalid first byte
            return false;
        }
        u = ch & (bit - 1);
        do {
            if (!input_get_byte(&ch)) {
                return false;
            }
            if (ch >> 6 != 2) {
                return false;
            }
            u = (u << 6) | (ch & 0x3f);
        } while (--count);
        *key = u;
    } else {
        switch (ch) {
        case '\t':
            *key = ch;
            break;
        case '\r':
            *key = KEY_ENTER;
            break;
        case 0x7f:
            *key = MOD_CTRL | '?';
            break;
        default:
            if (ch < 0x20) {
                // Control character
                *key = MOD_CTRL | ch | 0x40;
            } else {
                *key = ch;
            }
        }
    }
    return true;
}

static bool is_text(const char *str, int len)
{
    for (int i = 0; i < len; i++) {
        // NOTE: must be unsigned!
        unsigned char ch = str[i];
        switch (ch) {
        case '\t':
        case '\n':
        case '\r':
            break;
        default:
            if (ch < 0x20 || ch == 0x7f) {
                return false;
            }
            break;
        }
    }
    return true;
}

static bool read_key(int *key)
{
    if (!input_buf_fill && !fill_buffer()) {
        return false;
    }

    if (input_buf_fill > 4 && is_text(input_buf, input_buf_fill)) {
        *key = KEY_PASTE;
        return true;
    }
    if (input_buf[0] == '\033') {
        if (input_buf_fill > 1 || input_can_be_truncated) {
            if (read_special(key)) {
                return true;
            }
        }
        if (input_buf_fill == 1) {
            // Sometimes alt-key gets split into two reads
            fill_buffer_timeout();

            if (input_buf_fill > 1 && input_buf[1] == '\033') {
                /*
                 * Double-esc (+ maybe some other characters)
                 *
                 * Treat the first esc as a single key to make
                 * things like arrow keys work immediately after
                 * leaving (esc) the command line.
                 *
                 * Special key can't start with double-esc so this
                 * should be safe.
                 *
                 * This breaks the esc-key == alt-key rule for the
                 * esc-esc case but it shouldn't matter.
                 */
                return read_simple(key);
            }
        }
        if (input_buf_fill > 1) {
            // Unknown escape sequence or 'esc key' / 'alt-key'
            unsigned char ch;
            bool ok;

            // Throw escape away
            input_get_byte(&ch);
            ok = read_simple(key);
            if (!ok) {
                return false;
            }
            if (input_buf_fill == 0 || input_buf[0] == '\033') {
                // 'esc key' or 'alt-key'
                *key |= MOD_META;
                return true;
            }
            // Unknown escape sequence, avoid inserting it
            input_buf_fill = 0;
            return false;
        }
    }
    return read_simple(key);
}

bool term_read_key(int *key)
{
    bool ok = read_key(key);
    int k = *key;
    if (DEBUG > 2 && ok && k != KEY_PASTE && k > KEY_UNICODE_MAX) {
        // Modifiers and/or special key
        char *str = key_to_string(k);
        d_print("key: %s\n", str);
        free(str);
    }
    return ok;
}

char *term_read_paste(long *size)
{
    long alloc = ROUND_UP(input_buf_fill + 1, 1024);
    long count = 0;
    char *buf = xmalloc(alloc);

    if (input_buf_fill) {
        memcpy(buf, input_buf, input_buf_fill);
        count = input_buf_fill;
        input_buf_fill = 0;
    }
    while (1) {
        struct timeval tv = {
            .tv_sec = 0,
            .tv_usec = 0
        };
        fd_set set;
        int rc;

        FD_ZERO(&set);
        FD_SET(0, &set);
        rc = select(1, &set, NULL, NULL, &tv);
        if (rc < 0 && errno == EINTR) {
            continue;
        }
        if (rc <= 0) {
            break;
        }

        if (alloc - count < 256) {
            alloc *= 2;
            xrenew(buf, alloc);
        }
        do {
            rc = read(0, buf + count, alloc - count);
        } while (rc < 0 && errno == EINTR);
        if (rc <= 0) {
            break;
        }
        count += rc;
    }
    for (int i = 0; i < count; i++) {
        if (buf[i] == '\r') {
            buf[i] = '\n';
        }
    }
    *size = count;
    return buf;
}

void term_discard_paste(void)
{
    long size;
    free(term_read_paste(&size));
}

int term_get_size(int *w, int *h)
{
    struct winsize ws;

    if (ioctl(0, TIOCGWINSZ, &ws) != -1) {
        *w = ws.ws_col;
        *h = ws.ws_row;
        return 0;
    }
    return -1;
}

static void buffer_color(char x, unsigned char color)
{
    buffer[buffer_pos++] = ';';
    buffer[buffer_pos++] = x;
    if (color < 8) {
        buffer[buffer_pos++] = '0' + color;
    } else {
        buffer[buffer_pos++] = '8';
        buffer[buffer_pos++] = ';';
        buffer[buffer_pos++] = '5';
        buffer[buffer_pos++] = ';';
        buffer_num(color);
    }
}

const char *term_set_color(const struct term_color *color)
{
    struct term_color c = *color;

    // TERM=xterm: 8 colors
    // TERM=linux: 8 colors. colors > 7 corrupt screen
    if (term_cap.colors < 16 && c.fg >= 8 && c.fg <= 15) {
        c.attr |= ATTR_BOLD;
        c.fg &= 7;
    }

    // Max 35 bytes (3 + 6 * 2 + 2 * 9 + 2)
    buffer_pos = 0;
    buffer[buffer_pos++] = '\033';
    buffer[buffer_pos++] = '[';
    buffer[buffer_pos++] = '0';

    if (c.attr & ATTR_BOLD) {
        buffer[buffer_pos++] = ';';
        buffer[buffer_pos++] = '1';
    }
    if (c.attr & ATTR_LOW_INTENSITY) {
        buffer[buffer_pos++] = ';';
        buffer[buffer_pos++] = '2';
    }
    if (c.attr & ATTR_ITALIC) {
        buffer[buffer_pos++] = ';';
        buffer[buffer_pos++] = '3';
    }
    if (c.attr & ATTR_UNDERLINE) {
        buffer[buffer_pos++] = ';';
        buffer[buffer_pos++] = '4';
    }
    if (c.attr & ATTR_BLINKING) {
        buffer[buffer_pos++] = ';';
        buffer[buffer_pos++] = '5';
    }
    if (c.attr & ATTR_REVERSE_VIDEO) {
        buffer[buffer_pos++] = ';';
        buffer[buffer_pos++] = '7';
    }
    if (c.attr & ATTR_INVISIBLE_TEXT) {
        buffer[buffer_pos++] = ';';
        buffer[buffer_pos++] = '8';
    }
    if (c.fg >= 0) {
        buffer_color('3', c.fg);
    }
    if (c.bg >= 0) {
        buffer_color('4', c.bg);
    }
    buffer[buffer_pos++] = 'm';
    buffer[buffer_pos++] = 0;
    return buffer;
}

const char *term_move_cursor(int x, int y)
{
    if (x < 0 || x >= 999 || y < 0 || y >= 999) {
        return "";
    }

    x++;
    y++;
    // Max 11 bytes
    buffer_pos = 0;
    buffer[buffer_pos++] = '\033';
    buffer[buffer_pos++] = '[';
    buffer_num(y);
    buffer[buffer_pos++] = ';';
    buffer_num(x);
    buffer[buffer_pos++] = 'H';
    buffer[buffer_pos++] = 0;
    return buffer;
}
