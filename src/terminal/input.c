#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include "input.h"
#include "terminal.h"
#include "editor.h"
#include "util/ascii.h"
#include "util/xmalloc.h"

static struct {
    char buf[256];
    size_t len;
    bool can_be_truncated;
} input;

static void consume_input(size_t len)
{
    input.len -= len;
    if (input.len) {
        memmove(input.buf, input.buf + len, input.len);

        // Keys are sent faster than we can read
        input.can_be_truncated = true;
    }
}

static bool fill_buffer(void)
{
    if (input.len == sizeof(input.buf)) {
        return false;
    }

    if (!input.len) {
        input.can_be_truncated = false;
    }

    size_t avail = sizeof(input.buf) - input.len;
    ssize_t rc = read(STDIN_FILENO, input.buf + input.len, avail);
    if (rc <= 0) {
        return false;
    }

    input.len += (size_t)rc;
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

    // The Clang static analyzer can't always determine that the
    // FD_ZERO() call above has initialized the fd_set -- in glibc
    // it's implemented via "__asm__ __volatile__".
    //
    // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.Assign)
    FD_SET(STDIN_FILENO, &set);

    int rc = select(1, &set, NULL, NULL, &tv);
    if (rc > 0 && fill_buffer()) {
        return true;
    }
    return false;
}

static bool input_get_byte(unsigned char *ch)
{
    if (!input.len && !fill_buffer()) {
        return false;
    }
    *ch = input.buf[0];
    consume_input(1);
    return true;
}

static bool read_special(KeyCode *key)
{
    ssize_t len = terminal.parse_key_sequence(input.buf, input.len, key);
    switch (len) {
    case -1:
        // Possibly truncated
        break;
    case 0:
        // No match
        return false;
    default:
        // Match
        consume_input(len);
        return true;
    }

    if (input.can_be_truncated && fill_buffer()) {
        return read_special(key);
    }

    return false;
}

static bool read_simple(KeyCode *key)
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
        CodePoint bit = 1 << 6;
        int count = 0;

        while (ch & bit) {
            bit >>= 1;
            count++;
        }
        if (count == 0 || count > 3) {
            // Invalid first byte
            return false;
        }
        CodePoint u = ch & (bit - 1);
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
        *key = keycode_normalize(ch);
    }
    return true;
}

static bool is_text(const char *str, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (ascii_is_nonspace_cntrl(str[i])) {
            return false;
        }
    }
    return true;
}

bool term_read_key(KeyCode *key)
{
    if (!input.len && !fill_buffer()) {
        return false;
    }

    if (input.len > 4 && is_text(input.buf, input.len)) {
        *key = KEY_PASTE;
        return true;
    }

    if (input.buf[0] == '\033') {
        if (input.len > 1 || input.can_be_truncated) {
            if (read_special(key)) {
                return true;
            }
        }
        if (input.len == 1) {
            // Sometimes alt-key gets split into two reads
            fill_buffer_timeout();
            if (input.len > 1 && input.buf[1] == '\033') {
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
        if (input.len > 1) {
            // Unknown escape sequence or 'esc key' / 'alt-key'

            // Throw escape away
            consume_input(1);

            const bool ok = read_simple(key);
            if (!ok) {
                return false;
            }

            if (input.len == 0 || input.buf[0] == '\033') {
                // 'esc key' or 'alt-key'
                *key |= MOD_META;
                return true;
            }

            // Unknown escape sequence; avoid inserting it
            input.len = 0;
            return false;
        }
    }

    return read_simple(key);
}

char *term_read_paste(size_t *size)
{
    size_t alloc = round_size_to_next_multiple(input.len + 1, 1024);
    size_t count = 0;
    char *buf = xmalloc(alloc);

    if (input.len) {
        memcpy(buf, input.buf, input.len);
        count = input.len;
        input.len = 0;
    }

    while (1) {
        struct timeval tv = {
            .tv_sec = 0,
            .tv_usec = 0
        };

        fd_set set;
        FD_ZERO(&set);

        // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.Assign)
        FD_SET(STDIN_FILENO, &set);

        int rc = select(1, &set, NULL, NULL, &tv);
        if (rc < 0 && errno == EINTR) {
            continue;
        } else if (rc <= 0) {
            break;
        }

        if (alloc - count < 256) {
            alloc *= 2;
            xrenew(buf, alloc);
        }

        ssize_t n;
        do {
            n = read(STDIN_FILENO, buf + count, alloc - count);
        } while (n < 0 && errno == EINTR);

        if (n <= 0) {
            break;
        }
        count += n;
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
