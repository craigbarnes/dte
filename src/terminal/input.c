#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include "input.h"
#include "util/ascii.h"
#include "util/str-util.h"
#include "util/unicode.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"

static void consume_input(TermInputBuffer *input, size_t len)
{
    input->len -= len;
    if (input->len) {
        memmove(input->buf, input->buf + len, input->len);

        // Keys are sent faster than we can read
        input->can_be_truncated = true;
    }
}

static bool fill_buffer(TermInputBuffer *input)
{
    if (input->len == sizeof(input->buf)) {
        return false;
    }

    if (!input->len) {
        input->can_be_truncated = false;
    }

    size_t avail = sizeof(input->buf) - input->len;
    ssize_t rc = read(STDIN_FILENO, input->buf + input->len, avail);
    if (unlikely(rc <= 0)) {
        return false;
    }

    input->len += (size_t)rc;
    return true;
}

static bool fill_buffer_timeout(TermInputBuffer *input, unsigned int esc_timeout)
{
    struct timeval tv = {
        .tv_sec = esc_timeout / 1000,
        .tv_usec = (esc_timeout % 1000) * 1000
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
    if (rc > 0 && fill_buffer(input)) {
        return true;
    }
    return false;
}

static bool input_get_byte(TermInputBuffer *input, unsigned char *ch)
{
    if (!input->len && !fill_buffer(input)) {
        return false;
    }
    *ch = input->buf[0];
    consume_input(input, 1);
    return true;
}

static KeyCode read_special(Terminal *term)
{
    TermInputBuffer *input = &term->ibuf;
    KeyCode key;
    ssize_t len = term->parse_key_sequence(input->buf, input->len, &key);
    if (likely(len > 0)) {
        // Match
        consume_input(input, len);
        return key;
    }

    if (len < 0 && input->can_be_truncated && fill_buffer(input)) {
        // Possibly truncated
        return read_special(term);
    }

    // No match
    return KEY_NONE;
}

static KeyCode read_simple(TermInputBuffer *input)
{
    unsigned char ch = 0;

    // > 0 bytes in buf
    input_get_byte(input, &ch);

    // Normal key
    if (ch < 0x80) {
        return keycode_normalize(ch);
    }

    /*
     * 10xx xxxx invalid
     * 110x xxxx valid
     * 1110 xxxx valid
     * 1111 0xxx valid
     * 1111 1xxx invalid
     */
    CodePoint bit = 1 << 6;
    unsigned int count = 0;
    while (ch & bit) {
        bit >>= 1;
        count++;
    }

    if (count == 0 || count > 3) {
        // Invalid first byte
        return KEY_NONE;
    }

    CodePoint u = ch & (bit - 1);
    do {
        if (!input_get_byte(input, &ch) || ch >> 6 != 2) {
            return KEY_NONE;
        }
        u = (u << 6) | (ch & 0x3f);
    } while (--count);

    return u;
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

KeyCode term_read_key(Terminal *term, unsigned int esc_timeout)
{
    TermInputBuffer *input = &term->ibuf;
    if (!input->len && !fill_buffer(input)) {
        return KEY_NONE;
    }

    if (input->len > 4 && is_text(input->buf, input->len)) {
        return KEY_PASTE;
    }

    if (input->buf[0] == '\033') {
        if (input->len > 1 || input->can_be_truncated) {
            KeyCode key = read_special(term);
            if (key != KEY_NONE) {
                return key;
            }
        }
        if (input->len == 1) {
            // Sometimes alt-key gets split into two reads
            fill_buffer_timeout(input, esc_timeout);
            if (input->len > 1 && input->buf[1] == '\033') {
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
                return read_simple(input);
            }
        }
        if (input->len > 1) {
            // Unknown escape sequence or 'esc key' / 'alt-key'

            // Throw escape away
            consume_input(input, 1);

            KeyCode key = read_simple(input);
            if (key == KEY_NONE) {
                return KEY_NONE;
            }

            if (input->len == 0 || input->buf[0] == '\033') {
                // 'esc key' or 'alt-key'
                if (u_is_ascii_upper(key)) {
                    return MOD_META | MOD_SHIFT | ascii_tolower(key);
                }
                return MOD_META | key;
            }

            // Unknown escape sequence; avoid inserting it
            input->len = 0;
            return KEY_NONE;
        }
    }

    return read_simple(input);
}

char *term_read_paste(TermInputBuffer *input, size_t *size)
{
    size_t alloc = round_size_to_next_multiple(input->len + 1, 1024);
    size_t count = 0;
    char *buf = xmalloc(alloc);

    if (input->len) {
        memcpy(buf, input->buf, input->len);
        count = input->len;
        input->len = 0;
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

        ssize_t n = xread(STDIN_FILENO, buf + count, alloc - count);
        if (n <= 0) {
            break;
        }
        count += n;
    }

    strn_replace_byte(buf, count, '\r', '\n');
    *size = count;
    return buf;
}

void term_discard_paste(TermInputBuffer *input)
{
    size_t size;
    free(term_read_paste(input, &size));
}
