#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h> // NOLINT(portability-restrict-system-includes)
#include <sys/time.h> // NOLINT(portability-restrict-system-includes)
#include <unistd.h>
#include "input.h"
#include "feature.h"
#include "linux.h"
#include "output.h"
#include "parse.h"
#include "rxvt.h"
#include "trace.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/log.h"
#include "util/time-util.h"
#include "util/unicode.h"
#include "util/utf8.h"

static void consume_input(TermInputBuffer *input, size_t len)
{
    BUG_ON(len == 0);
    BUG_ON(len > input->len);

    if (log_trace_enabled(TRACEFLAG_INPUT)) {
        // Note that this occurs *after* e.g. query responses have been logged
        char buf[256];
        u_make_printable(input->buf, len, buf, sizeof(buf), MPF_C0_SYMBOLS);
        if (len == input->len) {
            TRACE_INPUT("consumed %zu bytes from input buffer: %s", len, buf);
        } else {
            TRACE_INPUT (
                "consumed %zu (of %u) bytes from input buffer: %s",
                len, input->len, buf
            );
        }
    }

    input->len -= len;
    if (input->len) {
        memmove(input->buf, input->buf + len, input->len);
        input->can_be_truncated = true; // Keys sent faster than we can read
    }
}

static bool fill_buffer(TermInputBuffer *input)
{
    if (unlikely(input->len == TERM_INBUF_SIZE)) {
        return false;
    }

    if (!input->len) {
        input->can_be_truncated = false;
    }

    size_t avail = TERM_INBUF_SIZE - input->len;
    ssize_t rc = read(STDIN_FILENO, input->buf + input->len, avail); // NOLINT(*-unsafe-functions)
    if (unlikely(rc <= 0)) {
        return false;
    }

    TRACE_INPUT("read %zu bytes into input buffer", (size_t)rc);
    input->len += (size_t)rc;
    return true;
}

static bool fill_buffer_timeout(TermInputBuffer *input, unsigned int esc_timeout_ms)
{
    struct timeval tv = {
        .tv_sec = esc_timeout_ms / MS_PER_SECOND,
        .tv_usec = (esc_timeout_ms % MS_PER_SECOND) * US_PER_MS
    };

    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    int rc = select(1, &set, NULL, NULL, &tv);
    return (rc > 0) && fill_buffer(input);
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

// Recursion via tail call; no overflow possible
// NOLINTNEXTLINE(misc-no-recursion)
static KeyCode read_special(TermInputBuffer *input, TermFeatureFlags features)
{
    KeyCode key;
    ssize_t len;
    if (unlikely(features & TFLAG_LINUX)) {
        len = linux_parse_key(input->buf, input->len, &key);
    } else if (unlikely(features & TFLAG_RXVT)) {
        len = rxvt_parse_key(input->buf, input->len, &key);
    } else {
        len = term_parse_sequence(input->buf, input->len, &key);
    }

    switch (len) {
    case TPARSE_PARTIAL_MATCH:
        if (input->can_be_truncated && fill_buffer(input)) {
            return read_special(input, features);
        }
        return KEY_NONE;
    case TPARSE_NO_MATCH:
        return KEY_NONE;
    }

    BUG_ON(len < 1);
    consume_input(input, len);
    return key;
}

/*
 * Get implied modifiers for the BS ('\b') character, depending on the
 * current terminal feature flags. Note that there's a flag for BS and
 * a separate flag for DEL (only 1 of which is ever active) so that
 * the absence of both causes both characters to produce a backspace
 * event (i.e. neither produce C-backspace). This is done because
 * wrongly interpreting Ctrl+Backspace as Backspace is merely annoying
 * (assuming they're bound to erase-word and erase respectively),
 * whereas the reverse becomes borderline unusable. It's also a safer
 * assumption in general, when no terminal-specific details are known.
 */
static KeyCode modifiers_for_bs(TermFeatureFlags flags)
{
    return (flags & TFLAG_BS_CTRL_BACKSPACE) ? MOD_CTRL : 0;
}

// Like modifiers_for_bs(), but for handling DEL ('\x7F')
static KeyCode modifiers_for_del(TermFeatureFlags flags)
{
    return (flags & TFLAG_DEL_CTRL_BACKSPACE) ? MOD_CTRL : 0;
}

static KeyCode read_simple(Terminal *term)
{
    TermInputBuffer *input = &term->ibuf;
    unsigned char ch = 0;

    // > 0 bytes in buf
    input_get_byte(input, &ch);

    if (ch < 0x80) {
        switch (ch) {
        case '\b': return KEY_BACKSPACE | modifiers_for_bs(term->features);
        case '\t': return KEY_TAB;
        case '\r': return KEY_ENTER;
        case 0x1B: return KEY_ESCAPE;
        case 0x7F: return KEY_BACKSPACE | modifiers_for_del(term->features);
        }
        return (ch >= 0x20) ? ch : MOD_CTRL | ascii_tolower(ch | 0x40);
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

static KeyCode term_read_input_legacy(Terminal *term, unsigned int esc_timeout_ms)
{
    TermInputBuffer *input = &term->ibuf;
    if (!input->len && !fill_buffer(input)) {
        return KEY_NONE;
    }

    if (input->len > 4 && is_text(input->buf, input->len)) {
        return KEYCODE_DETECTED_PASTE;
    }

    if (input->buf[0] != '\033') {
        return read_simple(term);
    }

    if (input->len > 1 || input->can_be_truncated) {
        KeyCode key = read_special(input, term->features);
        if (unlikely(key & KEYCODE_QUERY_REPLY_BIT)) {
            TermFeatureFlags features = key & ~KEYCODE_QUERY_REPLY_BIT;
            return term_handle_query_reply(term, features);
        }
        if (key == KEY_IGNORE) {
            return KEY_NONE;
        }
        if (key != KEY_NONE) {
            return key;
        }
    }

    if (input->len == 1) {
        // Sometimes alt-key gets split into two reads
        fill_buffer_timeout(input, esc_timeout_ms);
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
            return read_simple(term);
        }
    }

    if (input->len > 1) {
        // Unknown escape sequence or 'esc key' / 'alt-key'

        // Throw escape away
        consume_input(input, 1);

        KeyCode key = read_simple(term);
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
        consume_input(input, input->len);
        return KEY_NONE;
    }

    return read_simple(term);
}

/*
 This is similar to term_read_input_legacy(), but suitable only for
 terminals that support the Kitty keyboard protocol and with the
 following notable differences:
 • No function pointer indirection for parsing input from quirky
   terminals (term_parse_sequence() is always used)
 • No timing hacks to disambiguate the meaning of ESC bytes (since
   there is no ambiguity when using the Kitty protocol)
 • No additional code for parsing legacy encodings of Alt-modified
   key combos (i.e. those prefixed with extra ESC bytes)
 • No additional (timing dependent) code for treating ESC bytes as
   the Esc key (always encoded as `CSI 27 u` in the Kitty protocol)
 • No support for handling legacy style (undelimited) clipboard
   pastes (Kitty keyboard support is assumed to also imply bracketed
   paste support)
*/
static KeyCode term_read_input_modern(Terminal *term)
{
    TermInputBuffer *input = &term->ibuf;
    if (!input->len && !fill_buffer(input)) {
        return KEY_NONE;
    }

    if (input->buf[0] != '\033') {
        return read_simple(term);
    }

    KeyCode key;
    while (1) {
        ssize_t len = term_parse_sequence(input->buf, input->len, &key);
        if (len != TPARSE_PARTIAL_MATCH) {
            if (unlikely(len == TPARSE_NO_MATCH)) {
                // This return code only exists for term_read_input_legacy().
                // When using the Kitty protocol, this branch shouldn't ever
                // be taken (in practice), so we handle it as an edge case
                // by simply consuming (and ignoring) the ESC byte and the
                // byte that follows it.
                BUG_ON(input->len < 2); // Implied by the conditions above
                len = 2;
                key = KEY_IGNORE;
            }
            consume_input(input, len);
            break;
        }

        size_t avail = TERM_INBUF_SIZE - input->len;
        ssize_t rc = read(STDIN_FILENO, input->buf + input->len, avail); // NOLINT(*-unsafe-functions)
        if (unlikely(rc <= 0)) {
            LOG_ERRNO_ON(rc < 0, "read");
            return KEY_NONE;
        }

        TRACE_INPUT("read %zu bytes into input buffer", (size_t)rc);
        input->len += (size_t)rc;
    }

    if (unlikely(key & KEYCODE_QUERY_REPLY_BIT)) {
        TermFeatureFlags features = key & ~KEYCODE_QUERY_REPLY_BIT;
        return term_handle_query_reply(term, features);
    }

    return (key == KEY_IGNORE) ? KEY_NONE : key;
}

KeyCode term_read_input(Terminal *term, unsigned int esc_timeout_ms)
{
    if (term->features & TFLAG_KITTY_KEYBOARD) {
        return term_read_input_modern(term);
    }

    return term_read_input_legacy(term, esc_timeout_ms);
}
