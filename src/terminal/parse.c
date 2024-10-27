// Parser for escape sequences sent by terminals to clients.
// Copyright © Craig Barnes.
// SPDX-License-Identifier: GPL-2.0-only
// See also:
// • https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
// • https://sw.kovidgoyal.net/kitty/keyboard-protocol/
// • ECMA-48 §5.4 (https://ecma-international.org/publications-and-standards/standards/ecma-48/)
// • https://vt100.net/emu/dec_ansi_parser

#include "parse.h"
#include "query.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/log.h"
#include "util/unicode.h"

typedef enum {
    BYTE_CONTROL,       // 0x00..0x1F
    BYTE_INTERMEDIATE,  // 0x20..0x2F
    BYTE_PARAMETER,     // 0x30..0x3F
    BYTE_FINAL,         // 0x40..0x6F
    BYTE_FINAL_PRIVATE, // 0x70..0x7E
    BYTE_DELETE,        // 0x7F
    BYTE_OTHER,         // 0x80..0xFF
} ByteType;

static KeyCode decode_key_from_final_byte(uint8_t byte)
{
    switch (byte) {
    case 'A': return KEY_UP;
    case 'B': return KEY_DOWN;
    case 'C': return KEY_RIGHT;
    case 'D': return KEY_LEFT;
    case 'E': return KEY_BEGIN; // (keypad '5')
    case 'F': return KEY_END;
    case 'H': return KEY_HOME;
    case 'L': return KEY_INSERT;
    case 'M': return KEY_ENTER;
    case 'P': return KEY_F1;
    case 'Q': return KEY_F2;
    case 'R': return KEY_F3;
    case 'S': return KEY_F4;
    }
    return KEY_IGNORE;
}

static KeyCode decode_key_from_param(uint32_t param)
{
    switch (param) {
    case 1: return KEY_HOME;
    case 2: return KEY_INSERT;
    case 3: return KEY_DELETE;
    case 4: return KEY_END;
    case 5: return KEY_PAGE_UP;
    case 6: return KEY_PAGE_DOWN;
    case 7: return KEY_HOME;
    case 8: return KEY_END;
    case 11: return KEY_F1;
    case 12: return KEY_F2;
    case 13: return KEY_F3;
    case 14: return KEY_F4;
    case 15: return KEY_F5;
    case 17: return KEY_F6;
    case 18: return KEY_F7;
    case 19: return KEY_F8;
    case 20: return KEY_F9;
    case 21: return KEY_F10;
    case 23: return KEY_F11;
    case 24: return KEY_F12;
    case 25: return KEY_F13;
    case 26: return KEY_F14;
    case 28: return KEY_F15;
    case 29: return KEY_F16;
    case 31: return KEY_F17;
    case 32: return KEY_F18;
    case 33: return KEY_F19;
    case 34: return KEY_F20;
    }
    return KEY_IGNORE;
}

static KeyCode decode_kitty_special_key(uint32_t n)
{
    switch (n) {
    case 57359: return KEY_SCROLL_LOCK;
    case 57361: return KEY_PRINT_SCREEN;
    case 57362: return KEY_PAUSE;
    case 57363: return KEY_MENU;
    case 57376: return KEY_F13;
    case 57377: return KEY_F14;
    case 57378: return KEY_F15;
    case 57379: return KEY_F16;
    case 57380: return KEY_F17;
    case 57381: return KEY_F18;
    case 57382: return KEY_F19;
    case 57383: return KEY_F20;
    case 57399: return '0';
    case 57400: return '1';
    case 57401: return '2';
    case 57402: return '3';
    case 57403: return '4';
    case 57404: return '5';
    case 57405: return '6';
    case 57406: return '7';
    case 57407: return '8';
    case 57408: return '9';
    case 57409: return '.';
    case 57410: return '/';
    case 57411: return '*';
    case 57412: return '-';
    case 57413: return '+';
    case 57414: return KEY_ENTER;
    case 57415: return '=';
    case 57417: return KEY_LEFT;
    case 57418: return KEY_RIGHT;
    case 57419: return KEY_UP;
    case 57420: return KEY_DOWN;
    case 57421: return KEY_PAGE_UP;
    case 57422: return KEY_PAGE_DOWN;
    case 57423: return KEY_HOME;
    case 57424: return KEY_END;
    case 57425: return KEY_INSERT;
    case 57426: return KEY_DELETE;
    case 57427: return KEY_BEGIN;
    }
    return KEY_IGNORE;
}

// See: https://sw.kovidgoyal.net/kitty/keyboard-protocol/#modifiers
// ----------------------------
// Shift     0b1         (1)
// Alt       0b10        (2)
// Ctrl      0b100       (4)
// Super     0b1000      (8)
// Hyper     0b10000     (16)
// Meta      0b100000    (32)
// Capslock  0b1000000   (64)
// Numlock   0b10000000  (128)
// ----------------------------
static KeyCode decode_modifiers(uint32_t n)
{
    n--;
    if (unlikely(n > 255)) {
        return KEY_IGNORE;
    }

    static_assert(1 == MOD_SHIFT >> KEYCODE_MODIFIER_OFFSET);
    static_assert(2 == MOD_META >> KEYCODE_MODIFIER_OFFSET);
    static_assert(4 == MOD_CTRL >> KEYCODE_MODIFIER_OFFSET);
    static_assert(8 == MOD_SUPER >> KEYCODE_MODIFIER_OFFSET);
    static_assert(16 == MOD_HYPER >> KEYCODE_MODIFIER_OFFSET);
    static_assert(31 == MOD_MASK >> KEYCODE_MODIFIER_OFFSET);

    // Decode Meta and/or Alt as MOD_META and ignore Capslock/Numlock
    KeyCode mods = (n & 31) | ((n & 32) >> 4);
    return mods << KEYCODE_MODIFIER_OFFSET;
}

// Normalize KeyCode values originating from `CSI u` sequences
static KeyCode normalize_csi_u_keycode(KeyCode mods, KeyCode key)
{
    if (u_is_ascii_upper(key)) {
        if (mods & MOD_CTRL) {
            // This is done only for the sake of (older versions of) iTerm2
            // See also:
            // • https://gitlab.com/craigbarnes/dte/-/issues/130#note_870592688
            // • https://gitlab.com/craigbarnes/dte/-/issues/130#note_864512674
            // • https://gitlab.com/gnachman/iterm2/-/issues/10017
            // • https://gitlab.com/gnachman/iterm2/-/commit/9cd0241afd0655024153c8730d5b3ed1fe41faf7
            // • https://gitlab.com/gnachman/iterm2/-/issues/7440#note_129599012
            key = ascii_tolower(key);
        }
    } else if (key >= 57344 && key <= 63743) {
        key = decode_kitty_special_key(key);
        if (unlikely(key == KEY_IGNORE)) {
            return KEY_IGNORE;
        }
    }

    return mods | keycode_normalize(key);
}

// Normalize KeyCode values originating from xterm-style "modifyOtherKeys"
// sequences (CSI 27 ; <modifiers> ; <key> ~)
static KeyCode normalize_csi_27_tilde_keycode(KeyCode mods, KeyCode key)
{
    if (u_is_ascii_upper(key) && (mods & MOD_SHIFT)) {
        if (mods == MOD_SHIFT) {
            mods = 0;
        } else if (mods & (MOD_CTRL | MOD_META)) {
            key = ascii_tolower(key);
        }
    }

    return mods | keycode_normalize(key);
}

static ssize_t parse_ss3(const char *buf, size_t length, size_t i, KeyCode *k)
{
    if (unlikely(i >= length)) {
        return TPARSE_PARTIAL_MATCH;
    }

    const char ch = buf[i++];
    KeyCode key = decode_key_from_final_byte(ch);
    if (key != KEY_IGNORE) {
        *k = key;
    } else if (ch == 'X') {
        *k = '=';
    } else if (ch == ' ') {
        *k = KEY_SPACE;
    } else if ((ch >= 'j' && ch <= 'y') || ch == 'I') {
        *k = ch - 64;
    } else {
        *k = KEY_IGNORE;
    }

    return i;
}

static ByteType get_byte_type(unsigned char byte)
{
    enum {
        C = BYTE_CONTROL,
        I = BYTE_INTERMEDIATE,
        P = BYTE_PARAMETER,
        F = BYTE_FINAL,
        f = BYTE_FINAL_PRIVATE,
        x = BYTE_OTHER,
    };

    // ECMA 48 divides bytes ("bit combinations") into rows of 16 columns.
    // The byte classifications mostly fall into their own rows:
    static const uint8_t rows[16] = {
        C, C, I, P, F, F, F, f,
        x, x, x, x, x, x, x, x
    };

    // ... with the exception of byte 127 (DEL), which falls into rows[7]
    // but isn't a final byte like the others in that row:
    if (unlikely(byte == 127)) {
        return BYTE_DELETE;
    }

    return rows[(byte >> 4) & 0xF];
}

#define UNHANDLED(var, ...) unhandled(var, __LINE__, __VA_ARGS__)

PRINTF(3)
static void unhandled(bool *var, int line, const char *fmt, ...)
{
    if (*var) {
        // Only log the first error in a sequence
        return;
    }

    *var = true;
    if (DEBUG >= 2) {
        va_list ap;
        va_start(ap, fmt);
        log_msgv(LOG_LEVEL_DEBUG, __FILE__, line, fmt, ap);
        va_end(ap);
    }
}

size_t term_parse_csi_params(const char *buf, size_t len, size_t i, TermControlParams *csi)
{
    size_t nparams = 0;
    size_t nr_intermediate = 0;
    size_t sub = 0;
    size_t digits = 0;
    uint32_t num = 0;
    bool have_subparams = false;
    bool err = false;

    while (i < len) {
        const char ch = buf[i++];
        switch (ch) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            num = (num * 10) + (ch - '0');
            if (unlikely(num > UNICODE_MAX_VALID_CODEPOINT)) {
                UNHANDLED(&err, "CSI param overflow");
            }
            digits++;
            continue;
        case ';':
            if (unlikely(nparams >= ARRAYLEN(csi->params))) {
                UNHANDLED(&err, "too many params in CSI sequence");
                continue;
            }
            csi->nsub[nparams] = sub + 1;
            csi->params[nparams++][sub] = num;
            num = 0;
            digits = 0;
            sub = 0;
            continue;
        case ':':
            if (unlikely(sub >= ARRAYLEN(csi->params[0]))) {
                UNHANDLED(&err, "too many sub-params in CSI sequence");
                continue;
            }
            csi->params[nparams][sub++] = num;
            num = 0;
            digits = 0;
            have_subparams = true;
            continue;
        }

        switch (get_byte_type(ch)) {
        case BYTE_INTERMEDIATE:
            if (unlikely(nr_intermediate >= ARRAYLEN(csi->intermediate))) {
                UNHANDLED(&err, "too many intermediate bytes in CSI sequence");
            } else {
                csi->intermediate[nr_intermediate++] = ch;
            }
            continue;
        case BYTE_FINAL:
        case BYTE_FINAL_PRIVATE:
            csi->final_byte = ch;
            if (digits > 0) {
                if (unlikely(
                    nparams >= ARRAYLEN(csi->params)
                    || sub >= ARRAYLEN(csi->params[0])
                )) {
                    UNHANDLED(&err, "too many params/sub-params in CSI sequence");
                } else {
                    csi->nsub[nparams] = sub + 1;
                    csi->params[nparams++][sub] = num;
                }
            }
            goto exit_loop;
        case BYTE_PARAMETER:
            // ECMA-48 §5.4.2: "bit combinations 03/12 to 03/15 are
            // reserved for future standardization except when used
            // as the first bit combination of the parameter string."
            // (03/12 to 03/15 == '<' to '?')
            UNHANDLED(&err, "unhandled CSI param byte: '%c'", ch);
            continue;
        case BYTE_CONTROL:
            switch (ch) {
            case 0x1B: // ESC
                // Don't consume ESC; it's the start of another sequence
                i--;
                // Fallthrough
            case 0x18: // CAN
            case 0x1A: // SUB
                UNHANDLED(&err, "CSI sequence cancelled by 0x%02hhx", ch);
                csi->final_byte = ch;
                goto exit_loop;
            }
            // Fallthrough
        case BYTE_DELETE:
            continue;
        case BYTE_OTHER:
            UNHANDLED(&err, "unhandled byte in CSI sequence: 0x%02hhx", ch);
            continue;
        }

        BUG("unhandled byte type");
        err = true;
    }

exit_loop:
    csi->nparams = nparams;
    csi->nr_intermediate = nr_intermediate;
    csi->have_subparams = have_subparams;
    csi->unhandled_bytes = err;
    return i;
}

static ssize_t parse_csi(const char *buf, size_t len, size_t i, KeyCode *k)
{
    TermControlParams csi = {.nparams = 0};
    bool maybe_query_reply = (i < len && buf[i] >= '<' && buf[i] <= '?');
    uint8_t param_prefix = maybe_query_reply ? buf[i] : 0;
    i = term_parse_csi_params(buf, len, i + (maybe_query_reply ? 1 : 0), &csi);

    if (unlikely(csi.final_byte == 0)) {
        BUG_ON(i < len);
        return TPARSE_PARTIAL_MATCH;
    }
    if (unlikely(csi.unhandled_bytes)) {
        goto ignore;
    }

    if (maybe_query_reply) {
        *k = parse_csi_query_reply(&csi, param_prefix);
        return i;
    }

    if (unlikely(csi.nr_intermediate)) {
        goto ignore;
    }

    /*
     * This handles the basic CSI u ("fixterms") encoding and also the
     * extended kitty keyboard encoding.
     *
     * • https://www.leonerd.org.uk/hacks/fixterms/
     * • https://sw.kovidgoyal.net/kitty/keyboard-protocol/
     * • https://invisible-island.net/xterm/manpage/xterm.html#VT100-Widget-Resources:formatOtherKeys
     *
     * kitty params: key:unshifted-key:base-layout-key ; mods:event-type ; text
     */
    KeyCode key, mods = 0;
    if (csi.final_byte == 'u') {
        if (unlikely(csi.nsub[0] > 3 || csi.nsub[1] > 2 || csi.nsub[2] > 1)) {
            // Don't allow unknown sub-params
            goto ignore;
        }
        // Use the "base layout key", if present
        key = csi.params[0][csi.nsub[0] == 3 ? 2 : 0];
        switch (csi.nparams) {
        case 3:
        case 2:
            if (unlikely(csi.params[1][1] > 2)) {
                // Key release event
                goto ignore;
            }
            mods = decode_modifiers(csi.params[1][0]);
            if (unlikely(mods == KEY_IGNORE)) {
                goto ignore;
            }
            // Fallthrough
        case 1:
            *k = normalize_csi_u_keycode(mods, key);
            return i;
        }
        goto ignore;
    }

    if (unlikely(csi.have_subparams)) {
        goto ignore;
    }

    switch (csi.final_byte) {
    case '~':
        switch (csi.nparams) {
        case 3:
            if (unlikely(csi.params[0][0] != 27)) {
                goto ignore;
            }
            mods = decode_modifiers(csi.params[1][0]);
            if (unlikely(mods == KEY_IGNORE)) {
                goto ignore;
            }
            // xterm-style modifyOtherKeys encoding
            key = csi.params[2][0];
            *k = normalize_csi_27_tilde_keycode(mods, key);
            return i;
        case 2:
            mods = decode_modifiers(csi.params[1][0]);
            if (unlikely(mods == KEY_IGNORE)) {
                goto ignore;
            }
            // Fallthrough
        case 1:
            key = decode_key_from_param(csi.params[0][0]);
            if (key == KEY_IGNORE) {
                if (csi.params[0][0] == 200 && mods == 0) {
                    *k = KEYCODE_BRACKETED_PASTE;
                    return i;
                }
                goto ignore;
            }
            *k = mods | key;
            return i;
        }
        goto ignore;
    case 't':
        *k = parse_xtwinops_query_reply(&csi);
        return i;
    case 'Z':
        if (unlikely(csi.nparams != 0)) {
            goto ignore;
        }
        *k = MOD_SHIFT | KEY_TAB;
        return i;
    }

    switch (csi.nparams) {
    case 2:
        mods = decode_modifiers(csi.params[1][0]);
        if (unlikely(mods == KEY_IGNORE || csi.params[0][0] != 1)) {
            goto ignore;
        }
        // Fallthrough
    case 0:
        key = decode_key_from_final_byte(csi.final_byte);
        if (unlikely(key == KEY_IGNORE)) {
            goto ignore;
        }
        *k = mods | key;
        return i;
    }

ignore:
    *k = KEY_IGNORE;
    return i;
}

static ssize_t parse_osc(const char *buf, size_t len, size_t i, KeyCode *k)
{
    char data[4096];
    for (size_t pos = 0; i < len; ) {
        unsigned char ch = buf[i++];
        if (unlikely(ch < 0x20)) {
            switch (ch) {
            case 0x18: // CAN
            case 0x1A: // SUB
                *k = KEY_IGNORE;
                return i;
            case 0x1B: // ESC (https://vt100.net/emu/dec_ansi_parser#STESC)
                i--;
                // Fallthrough
            case 0x07: // BEL
                *k = parse_osc_query_reply(data, pos, pos >= sizeof(data));
                return i;
            }
            continue;
        }
        // Collect 0x20..0xFF (UTF-8 allowed)
        if (likely(pos < sizeof(data))) {
            data[pos++] = ch;
        }
    }

    // Unterminated sequence (possibly truncated)
    return TPARSE_PARTIAL_MATCH;
}

static ssize_t parse_dcs(const char *buf, size_t len, size_t i, KeyCode *k)
{
    char data[4096];
    for (size_t pos = 0; i < len; ) {
        unsigned char ch = buf[i++];
        if (unlikely(ch < 0x20 || ch == 0x7F)) {
            switch (ch) {
            case 0x18: // CAN
            case 0x1A: // SUB
                *k = KEY_IGNORE;
                return i;
            case 0x1B: // ESC (https://vt100.net/emu/dec_ansi_parser#STESC)
                *k = parse_dcs_query_reply(data, pos, pos >= sizeof(data));
                return i - 1;
            }
            continue;
        }
        // Collect 0x20..0xFF (excluding 0x7F)
        if (likely(pos < sizeof(data))) {
            data[pos++] = ch;
        }
    }

    // Unterminated sequence (possibly truncated)
    return TPARSE_PARTIAL_MATCH;
}

/*
 * Some terminals emit output resembling CSI/SS3 sequences without properly
 * conforming to ECMA-48 §5.4, so we use an approach that accommodates
 * terminal-specific special cases. This more or less precludes the use of
 * a state machine (like e.g. the "dec_ansi_parser" mentioned at the top of
 * this file). There's no real standard for "terminal to host" communications,
 * although conforming to ECMA-48 §5.4 has been a de facto standard since the
 * DEC VTs, with only a few (unnecessary and most likely not deliberate)
 * exceptions in some emulators.
 *
 * See also: rxvt.c and linux.c
 */
ssize_t term_parse_sequence(const char *buf, size_t length, KeyCode *k)
{
    if (unlikely(length == 0 || buf[0] != '\033')) {
        return 0;
    } else if (unlikely(length == 1)) {
        return TPARSE_PARTIAL_MATCH;
    }

    switch (buf[1]) {
    case 'O': return parse_ss3(buf, length, 2, k);
    case 'P': return parse_dcs(buf, length, 2, k);
    case '[': return parse_csi(buf, length, 2, k);
    case ']': return parse_osc(buf, length, 2, k);
    // String Terminator (https://vt100.net/emu/dec_ansi_parser#STESC)
    case '\\': *k = KEY_IGNORE; return 2;
    }

    return 0;
}
