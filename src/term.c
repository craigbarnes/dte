#include "term.h"
#include "common.h"
#include "editor.h"

#undef CTRL

#include <sys/ioctl.h>
#include <termios.h>

TerminalInfo terminal;

static struct termios termios_save;

static char *escape_key(const char *key, size_t len)
{
    static char buf[1024];
    size_t j = 0;
    for (size_t i = 0; i < len && i < sizeof(buf) - 1; i++) {
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
static size_t input_buf_fill;
static bool input_can_be_truncated;

static void consume_input(size_t len)
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
    if (input_buf_fill == sizeof(input_buf)) {
        return false;
    }

    if (!input_buf_fill) {
        input_can_be_truncated = false;
    }

    int rc = read (
        STDIN_FILENO,
        input_buf + input_buf_fill,
        sizeof(input_buf) - input_buf_fill
    );
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

    FD_ZERO(&set);
    FD_SET(0, &set);
    int rc = select(1, &set, NULL, NULL, &tv);
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

static bool read_special(Key *key)
{
    bool possibly_truncated = false;

    if (DEBUG > 2) {
        d_print("keycode: '%s'\n", escape_key(input_buf, input_buf_fill));
    }

    for (size_t i = 0; i < terminal.keymap_length; i++) {
        const TermKeyMap *const km = &terminal.keymap[i];
        const char *const keycode = km->code;

        if (!keycode) {
            continue;
        }

        const size_t len = strlen(keycode);
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

    if (possibly_truncated && input_can_be_truncated && fill_buffer()) {
        return read_special(key);
    }

    return false;
}

static bool read_simple(Key *key)
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

static bool is_text(const char *const str, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        // NOTE: must be unsigned!
        const unsigned char ch = str[i];
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

static bool read_key(Key *key)
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

            // Throw escape away
            input_get_byte(&ch);
            const bool ok = read_simple(key);
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

bool term_read_key(Key *key)
{
    const bool ok = read_key(key);
    const Key k = *key;
    if (DEBUG > 2 && ok && k != KEY_PASTE && k > KEY_UNICODE_MAX) {
        // Modifiers and/or special key
        char *str = key_to_string(k);
        d_print("key: %s\n", str);
        free(str);
    }
    return ok;
}

char *term_read_paste(size_t *size)
{
    size_t alloc = ROUND_UP(input_buf_fill + 1, 1024);
    size_t count = 0;
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

        FD_ZERO(&set);
        FD_SET(0, &set);
        int rc = select(1, &set, NULL, NULL, &tv);
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
            rc = read(STDIN_FILENO, buf + count, alloc - count);
        } while (rc < 0 && errno == EINTR);
        if (rc <= 0) {
            break;
        }
        count += rc;
    }
    for (size_t i = 0; i < count; i++) {
        if (buf[i] == '\r') {
            buf[i] = '\n';
        }
    }
    *size = count;
    return buf;
}

void term_discard_paste(void)
{
    size_t size;
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
