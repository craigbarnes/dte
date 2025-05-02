#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h> // NOLINT(portability-restrict-system-includes)
#include <sys/time.h> // NOLINT(portability-restrict-system-includes)
#include <unistd.h>
#include "input.h"
#include "output.h"
#include "trace.h"
#include "util/ascii.h"
#include "util/bit.h"
#include "util/log.h"
#include "util/time-util.h"
#include "util/unicode.h"
#include "util/utf8.h"
#include "util/xmalloc.h"

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

// Recursion via tail call; no overflow possible
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
    case TFLAG_NO_QUERY_L3: return "NOQUERY3";
    case TFLAG_8_COLOR: return "C8";
    case TFLAG_16_COLOR: return "C16";
    case TFLAG_256_COLOR: return "C256";
    case TFLAG_TRUE_COLOR: return "TC";
    case TFLAG_MODIFY_OTHER_KEYS: return "MOKEYS";
    case TFLAG_BS_CTRL_BACKSPACE: return "BSCTRL";
    case TFLAG_DEL_CTRL_BACKSPACE: return "DELCTRL";
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

    if ((detected & TFLAG_QUERY_L3) && (existing & TFLAG_NO_QUERY_L3)) {
        LOG_INFO("terminal feature QUERY3 overridden by NOQUERY3");
        detected &= ~TFLAG_QUERY_L3;
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
        if (existing & TFLAG_MODIFY_OTHER_KEYS) {
            // Disable modifyOtherKeys mode, if previously enabled by
            // main() → ui_first_start() → term_enable_private_modes()
            // (i.e. due to feature flags set by term_init())
            term_put_literal(obuf, "\033[>4m");
        }
        // Enable Kitty Keyboard Protocol bits 1 and 4 (1|4 == 5). See also:
        // • https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#:~:text=CSI%20%3E%20Pp%20m
        // • https://sw.kovidgoyal.net/kitty/keyboard-protocol/#progressive-enhancement
        term_put_literal(obuf, "\033[>5u");
        flush = true;
    }

    if (
        is_newly_detected_feature(existing, detected, TFLAG_MODIFY_OTHER_KEYS)
        && !((existing | detected) & TFLAG_KITTY_KEYBOARD)
    ) {
        term_put_literal(obuf, "\033[>4;1m\033[>4;2m"); // modifyOtherKeys=1/2
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
        return read_simple(term);
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
