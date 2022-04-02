#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include "input.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/str-util.h"
#include "util/unicode.h"
#include "util/xmalloc.h"
#include "util/xmemmem.h"
#include "util/xreadwrite.h"

void term_input_init(TermInputBuffer *ibuf)
{
    *ibuf = (TermInputBuffer) {
        .buf = xmalloc(TERM_INBUF_SIZE)
    };
}

void term_input_free(TermInputBuffer *ibuf)
{
    free(ibuf->buf);
}

static void consume_input(TermInputBuffer *input, size_t len)
{
    BUG_ON(len > input->len);
    input->len -= len;
    if (input->len) {
        memmove(input->buf, input->buf + len, input->len);

        // Keys are sent faster than we can read
        input->can_be_truncated = true;
    }
}

static bool fill_buffer(TermInputBuffer *input)
{
    if (input->len == TERM_INBUF_SIZE) {
        return false;
    }

    if (!input->len) {
        input->can_be_truncated = false;
    }

    size_t avail = TERM_INBUF_SIZE - input->len;
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
        return KEY_DETECTED_PASTE;
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

static String term_read_detected_paste(TermInputBuffer *input)
{
    String str = string_new(4096 + input->len);
    if (input->len) {
        string_append_buf(&str, input->buf, input->len);
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

        string_reserve_space(&str, 4096);
        char *buf = str.buffer + str.len;
        ssize_t n = xread(STDIN_FILENO, buf, str.alloc - str.len);
        if (n <= 0) {
            break;
        }
        str.len += n;
    }

    strn_replace_byte(str.buffer, str.len, '\r', '\n');
    return str;
}

static String term_read_bracketed_paste(TermInputBuffer *input)
{
    size_t read_max = TERM_INBUF_SIZE;
    String str = string_new(read_max + input->len);
    if (input->len) {
        string_append_buf(&str, input->buf, input->len);
        input->len = 0;
    }

    static const StringView delim = STRING_VIEW("\033[201~");
    char *start, *end;
    ssize_t read_len;

    while (1) {
        string_reserve_space(&str, read_max);
        start = str.buffer + str.len;
        read_len = xread(STDIN_FILENO, start, read_max);
        if (unlikely(read_len <= 0)) {
            goto read_error;
        }
        end = xmemmem(start, read_len, delim.data, delim.length);
        if (end) {
            break;
        }
        str.len += read_len;
    }

    size_t final_chunk_len = (size_t)(end - start);
    str.len += final_chunk_len;
    final_chunk_len += delim.length;
    BUG_ON(final_chunk_len > read_len);

    size_t remainder = read_len - final_chunk_len;
    if (unlikely(remainder)) {
        // Copy anything still in the buffer beyond the end delimiter
        // into the normal input buffer
        LOG_INFO("remainder after bracketed paste: %zu", remainder);
        BUG_ON(remainder > TERM_INBUF_SIZE);
        memcpy(input->buf, start + final_chunk_len, remainder);
        input->len = remainder;
    }

    LOG_INFO("received bracketed paste of %zu bytes", str.len);
    strn_replace_byte(str.buffer, str.len, '\r', '\n');
    return str;

read_error:
    LOG_ERROR("read error: %s", strerror(errno));
    string_free(&str);
    BUG_ON(str.buffer);
    return str;
}

String term_read_paste(TermInputBuffer *input, bool bracketed)
{
    if (bracketed) {
        return term_read_bracketed_paste(input);
    }
    return term_read_detected_paste(input);
}

void term_discard_paste(TermInputBuffer *input, bool bracketed)
{
    String str = term_read_paste(input, bracketed);
    string_free(&str);
    LOG_INFO("%spaste discarded", bracketed ? "bracketed " : "");
}
