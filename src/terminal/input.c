#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h> // NOLINT(portability-restrict-system-includes)
#include <sys/time.h> // NOLINT(portability-restrict-system-includes)
#include <unistd.h>
#include "input.h"
#include "output.h"
#include "util/ascii.h"
#include "util/bit.h"
#include "util/debug.h"
#include "util/log.h"
#include "util/time-util.h"
#include "util/unicode.h"
#include "util/utf8.h"
#include "util/xmalloc.h"

void term_input_init(TermInputBuffer *ibuf)
{
    *ibuf = (TermInputBuffer) {
        .buf = xmalloc(TERM_INBUF_SIZE)
    };
}

void term_input_free(TermInputBuffer *ibuf)
{
    free(ibuf->buf);
    *ibuf = (TermInputBuffer){.buf = NULL};
}

static void consume_input(TermInputBuffer *input, size_t len)
{
    BUG_ON(len == 0);
    BUG_ON(len > input->len);

    if (log_level_trace_enabled()) {
        // Note that this occurs *after* e.g. query responses have been logged
        char buf[256];
        u_make_printable(input->buf, len, buf, sizeof(buf), MPF_C0_SYMBOLS);
        if (len == input->len) {
            LOG_TRACE("consumed %zu bytes from input buffer: %s", len, buf);
        } else {
            LOG_TRACE (
                "consumed %zu (of %zu) bytes from input buffer: %s",
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

    LOG_TRACE("read %zu bytes into input buffer", (size_t)rc);
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

// NOLINTNEXTLINE(misc-no-recursion)
static KeyCode read_special(Terminal *term)
{
    TermInputBuffer *input = &term->ibuf;
    KeyCode key;
    ssize_t len = term->parse_input(input->buf, input->len, &key);
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

    if (ch < 0x80) {
        // TODO: Interpret \b or 0x7F as MOD_CTRL|KEY_BACKSPACE, when appropriate
        // (e.g. based on XTGETTCAP "kbs" or DECRQM DECBKM response)
        switch (ch) {
        case '\b': return KEY_BACKSPACE;
        case '\t': return KEY_TAB;
        case '\r': return KEY_ENTER;
        case 0x1B: return KEY_ESCAPE;
        case 0x7F: return KEY_BACKSPACE;
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

static const char *tflag_to_str(TermFeatureFlags flag)
{
    // This function only handles single flags. Values with multiple
    // bits set should be handled as appropriate, by the caller.
    WARN_ON(!IS_POWER_OF_2(flag));

    switch ((unsigned int)flag) {
    case TFLAG_BACK_COLOR_ERASE: return "BCE";
    case TFLAG_ECMA48_REPEAT: return "REP";
    case TFLAG_SET_WINDOW_TITLE: return "TITLE";
    case TFLAG_OSC52_COPY: return "OSC52";
    case TFLAG_META_ESC: return "METAESC";
    case TFLAG_ALT_ESC: return "ALTESC";
    case TFLAG_KITTY_KEYBOARD: return "KITTYKBD";
    case TFLAG_SYNC: return "SYNC";
    case TFLAG_QUERY_L2: return "QUERY2";
    case TFLAG_QUERY_L3: return "QUERY3";
    case TFLAG_8_COLOR: return "C8";
    case TFLAG_16_COLOR: return "C16";
    case TFLAG_256_COLOR: return "C256";
    case TFLAG_TRUE_COLOR: return "TC";
    }

    return "??";
}

static COLD void log_detected_features (
    TermFeatureFlags existing,
    TermFeatureFlags detected
) {
    // Don't log QUERY flags more than once
    const TermFeatureFlags query_flags = TFLAG_QUERY_L2 | TFLAG_QUERY_L3;
    const TermFeatureFlags repeat_query_flags = query_flags & detected & existing;
    detected &= ~repeat_query_flags;

    while (detected) {
        // Iterate through detected features, by finding the least
        // significant set bit and then logging and unsetting it
        TermFeatureFlags flag = u32_lsbit(detected);
        const char *name = tflag_to_str(flag);
        const char *extra = (existing & flag) ? " (was already set)" : "";
        LOG_INFO("terminal feature %s detected via query%s", name, extra);
        detected &= ~flag;
    }
}

static bool is_newly_detected_feature (
    TermFeatureFlags existing,
    TermFeatureFlags detected,
    TermFeatureFlags feature
) {
    BUG_ON(!IS_POWER_OF_2(feature));
    // (detected & feature) && !(existing & feature)
    return ~existing & detected & feature;
}

UNITTEST {
    BUG_ON(is_newly_detected_feature(1, 1, 1));
    BUG_ON(is_newly_detected_feature(1, 2, 1));
    BUG_ON(is_newly_detected_feature(3, 1, 1));
    BUG_ON(is_newly_detected_feature(1, 3, 1));
    BUG_ON(is_newly_detected_feature(0, 6, 1));
    BUG_ON(!is_newly_detected_feature(0, 1, 1));
    BUG_ON(!is_newly_detected_feature(2, 1, 1));
    BUG_ON(!is_newly_detected_feature(3, 4, 4));
}

static KeyCode handle_query_reply(Terminal *term, KeyCode key)
{
    const TermFeatureFlags existing = term->features;
    TermFeatureFlags detected = key & ~KEYCODE_QUERY_REPLY_BIT;
    if (unlikely(log_level_enabled(LOG_LEVEL_INFO))) {
        log_detected_features(existing, detected);
    }

    TermFeatureFlags escflags = TFLAG_META_ESC | TFLAG_ALT_ESC;
    if ((detected & escflags) && (existing & TFLAG_KITTY_KEYBOARD)) {
        const char *name = tflag_to_str(detected);
        const char *ovr = tflag_to_str(TFLAG_KITTY_KEYBOARD);
        LOG_INFO("terminal feature %s overridden by %s", name, ovr);
        detected &= ~escflags;
    }

    TermOutputBuffer *obuf = &term->obuf;
    bool flush = false;
    if (is_newly_detected_feature(existing, detected, TFLAG_QUERY_L2)) {
        term_put_level_2_queries(term, false);
        flush = true;
    }
    if (is_newly_detected_feature(existing, detected, TFLAG_QUERY_L3)) {
        term_put_level_3_queries(term, false);
        flush = true;
    }

    if (is_newly_detected_feature(existing, detected, TFLAG_KITTY_KEYBOARD)) {
        // First disable modifyOtherKeys mode (as previously enabled by
        // main() → ui_start() → term_enable_private_modes()) and then
        // enable Kitty Keyboard Protocol bits 1 and 4 (1|4 == 5).
        // See also:
        // • https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#:~:text=CSI%20%3E%20Pp%20m
        // • https://sw.kovidgoyal.net/kitty/keyboard-protocol/#progressive-enhancement
        term_put_literal(obuf, "\033[>4m\033[>5u");
        flush = true;
    }

    if (is_newly_detected_feature(existing, detected, TFLAG_META_ESC)) {
        term_put_literal(obuf, "\033[?1036h"); // DECSET 1036 (metaSendsEscape)
        flush = true;
    }
    if (is_newly_detected_feature(existing, detected, TFLAG_ALT_ESC)) {
        term_put_literal(obuf, "\033[?1039h"); // DECSET 1039 (altSendsEscape)
        flush = true;
    }

    if (flush) {
        term_output_flush(obuf);
    }

    term->features |= detected;
    return KEY_NONE;
}

KeyCode term_read_input(Terminal *term, unsigned int esc_timeout_ms)
{
    TermInputBuffer *input = &term->ibuf;
    if (!input->len && !fill_buffer(input)) {
        return KEY_NONE;
    }

    if (input->len > 4 && is_text(input->buf, input->len)) {
        return KEYCODE_DETECTED_PASTE;
    }

    if (input->buf[0] != '\033') {
        return read_simple(input);
    }

    if (input->len > 1 || input->can_be_truncated) {
        KeyCode key = read_special(term);
        if (unlikely(key & KEYCODE_QUERY_REPLY_BIT)) {
            return handle_query_reply(term, key);
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
        consume_input(input, input->len);
        return KEY_NONE;
    }

    return read_simple(input);
}
