// Parser for escape sequences sent by terminals to clients.
// Copyright 2018-2024 Craig Barnes.
// SPDX-License-Identifier: GPL-2.0-only
// See also:
// - https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
// - https://sw.kovidgoyal.net/kitty/keyboard-protocol/
// - ECMA-48 §5.4 (https://ecma-international.org/publications-and-standards/standards/ecma-48/)

#include <stdbool.h>
#include <stdint.h>
#include "parse.h"
#include "terminal.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/log.h"
#include "util/string-view.h"
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

// https://vt100.net/docs/vt510-rm/DECRPM
typedef enum {
    DECRPM_NOT_RECOGNIZED = 0,
    DECRPM_SET = 1,
    DECRPM_RESET = 2,
    DECRPM_PERMANENTLY_SET = 3,
    DECRPM_PERMANENTLY_RESET = 4,
} TermPrivateModeStatus;

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
    return KEY_NONE;
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
    return KEY_NONE;
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

static KeyCode decode_modifiers(uint32_t n)
{
    n--;
    if (unlikely(n > 15)) {
        return 0;
    }

    static_assert(1 == MOD_SHIFT >> KEYCODE_MODIFIER_OFFSET);
    static_assert(2 == MOD_META >> KEYCODE_MODIFIER_OFFSET);
    static_assert(4 == MOD_CTRL >> KEYCODE_MODIFIER_OFFSET);

    // Decode Meta (bit 4) and/or Alt (bit 2) as MOD_META
    KeyCode mods = (n & 7) | ((n & 8) >> 2);
    return mods << KEYCODE_MODIFIER_OFFSET;
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
static KeyCode decode_extended_modifiers(uint32_t n)
{
    n--;
    if (unlikely(n > 255)) {
        return 0;
    }

    static_assert(8 == MOD_SUPER >> KEYCODE_MODIFIER_OFFSET);
    static_assert(16 == MOD_HYPER >> KEYCODE_MODIFIER_OFFSET);
    static_assert(31 == MOD_MASK >> KEYCODE_MODIFIER_OFFSET);

    // Decode Meta and/or Alt as MOD_META and ignore Capslock/Numlock
    KeyCode mods = (n & 31) | ((n & 32) >> 4);
    return mods << KEYCODE_MODIFIER_OFFSET;
}

static KeyCode normalize_extended_keycode(KeyCode mods, KeyCode key)
{
    if (u_is_ascii_upper(key)) {
        if (mods & MOD_CTRL) {
            key = ascii_tolower(key);
        }
    } else if (key >= 57344 && key <= 63743) {
        key = decode_kitty_special_key(key);
    }

    return mods | keycode_normalize(key);
}

static ssize_t parse_ss3(const char *buf, size_t length, size_t i, KeyCode *k)
{
    if (unlikely(i >= length)) {
        return -1;
    }

    const char ch = buf[i++];
    KeyCode key = decode_key_from_final_byte(ch);
    if (key) {
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

typedef struct {
    uint32_t params[4][4];
    uint8_t nsub[4]; // lengths for params arrays (sub-params)
    uint8_t intermediate[4];
    uint8_t nparams;
    uint8_t nr_intermediate;
    uint8_t final_byte;
    bool have_subparams;
    bool unhandled_bytes;
} ControlParams;

static size_t parse_csi_params(const char *buf, size_t len, size_t i, ControlParams *csi)
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
                UNHANDLED(&err, "Too many params in CSI sequence");
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
                UNHANDLED(&err, "Too many sub-params in CSI sequence");
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
                UNHANDLED(&err, "Too many intermediate bytes in CSI sequence");
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
                    UNHANDLED(&err, "Too many params/sub-params in CSI sequence");
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
            UNHANDLED(&err, "Unhandled CSI param byte: '%c'", ch);
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
            UNHANDLED(&err, "Unhandled byte in CSI sequence: 0x%02hhx", ch);
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

// NOLINTNEXTLINE(readability-function-size)
UNITTEST {
    ControlParams csi = {.nparams = 0};
    StringView s = STRING_VIEW("\033[901;0;55mx");
    size_t n = parse_csi_params(s.data, s.length, 2, &csi);
    BUG_ON(n != s.length - 1);
    BUG_ON(csi.nparams != 3);
    BUG_ON(csi.params[0][0] != 901);
    BUG_ON(csi.params[1][0] != 0);
    BUG_ON(csi.params[2][0] != 55);
    BUG_ON(csi.nr_intermediate != 0);
    BUG_ON(csi.final_byte != 'm');
    BUG_ON(csi.have_subparams);
    BUG_ON(csi.unhandled_bytes);

    csi = (ControlParams){.nparams = 0};
    s = strview_from_cstring("\033[123;09;56:78:99m");
    n = parse_csi_params(s.data, s.length, 2, &csi);
    BUG_ON(n != s.length);
    BUG_ON(csi.nparams != 3);
    static_assert(ARRAYLEN(csi.nsub) >= 4);
    static_assert(ARRAYLEN(csi.nsub) == ARRAYLEN(csi.params));
    BUG_ON(csi.nsub[0] != 1);
    BUG_ON(csi.nsub[1] != 1);
    BUG_ON(csi.nsub[2] != 3);
    BUG_ON(csi.nsub[3] != 0);
    BUG_ON(csi.params[0][0] != 123);
    BUG_ON(csi.params[1][0] != 9);
    BUG_ON(csi.params[2][0] != 56);
    BUG_ON(csi.params[2][1] != 78);
    BUG_ON(csi.params[2][2] != 99);
    BUG_ON(csi.params[3][0] != 0);
    BUG_ON(csi.nr_intermediate != 0);
    BUG_ON(csi.final_byte != 'm');
    BUG_ON(!csi.have_subparams);
    BUG_ON(csi.unhandled_bytes);

    csi = (ControlParams){.nparams = 0};
    s = strview_from_cstring("\033[1:2:3;44:55;;6~");
    n = parse_csi_params(s.data, s.length, 2, &csi);
    BUG_ON(n != s.length);
    BUG_ON(csi.nparams != 4);
    BUG_ON(csi.nsub[0] != 3);
    BUG_ON(csi.nsub[1] != 2);
    BUG_ON(csi.nsub[2] != 1);
    BUG_ON(csi.params[0][0] != 1);
    BUG_ON(csi.params[0][1] != 2);
    BUG_ON(csi.params[0][2] != 3);
    BUG_ON(csi.params[1][0] != 44);
    BUG_ON(csi.params[1][1] != 55);
    BUG_ON(csi.params[2][0] != 0);
    BUG_ON(csi.params[3][0] != 6);
    BUG_ON(csi.nr_intermediate != 0);
    BUG_ON(csi.final_byte != '~');
    BUG_ON(!csi.have_subparams);
    BUG_ON(csi.unhandled_bytes);

    csi = (ControlParams){.nparams = 0};
    s = strview_from_cstring("\033[+2p");
    n = parse_csi_params(s.data, s.length, 2, &csi);
    BUG_ON(n != s.length);
    BUG_ON(csi.nparams != 1);
    BUG_ON(csi.nsub[0] != 1);
    BUG_ON(csi.params[0][0] != 2);
    BUG_ON(csi.nr_intermediate != 1);
    BUG_ON(csi.intermediate[0] != '+');
    BUG_ON(csi.final_byte != 'p');
    BUG_ON(csi.have_subparams);
    BUG_ON(csi.unhandled_bytes);

    csi = (ControlParams){.nparams = 0};
    s = strview_from_cstring("\033[?47;1$y");
    n = parse_csi_params(s.data, s.length, 2, &csi);
    BUG_ON(n != s.length);
    BUG_ON(csi.nparams != 2);
    BUG_ON(csi.params[0][0] != 47);
    BUG_ON(csi.params[1][0] != 1);
    BUG_ON(csi.nr_intermediate != 1);
    BUG_ON(csi.intermediate[0] != '$');
    BUG_ON(csi.have_subparams);
    // Note: question mark param prefixes are handled in parse_csi() instead
    // of parse_csi_params(), so the latter reports it as an unhandled byte
    BUG_ON(!csi.unhandled_bytes);
}

static const char *decrpm_status_to_str(TermPrivateModeStatus val)
{
    switch (val) {
    case DECRPM_NOT_RECOGNIZED: return "not recognized";
    case DECRPM_SET: return "set";
    case DECRPM_RESET: return "reset";
    case DECRPM_PERMANENTLY_SET: return "permanently set";
    case DECRPM_PERMANENTLY_RESET: return "permanently reset";
    }
    return "INVALID";
}

static bool decrpm_is_set_or_reset(TermPrivateModeStatus status)
{
    return status == DECRPM_SET || status == DECRPM_RESET;
}

static KeyCode parse_csi_query_reply(const ControlParams *csi)
{
    if (unlikely(csi->have_subparams || csi->nr_intermediate > 1)) {
        goto ignore;
    }

    uint8_t final = csi->final_byte;
    uint8_t nparams = csi->nparams;
    uint8_t intermediate = csi->nr_intermediate ? csi->intermediate[0] : 0;

    if (final == 'y' && intermediate == '$' && nparams == 2) {
        // DECRPM reply to DECRQM query (CSI ? Ps; Pm $ y)
        unsigned int mode = csi->params[0][0];
        unsigned int status = csi->params[1][0];
        const char *desc = decrpm_status_to_str(status);
        LOG_DEBUG("DECRPM reply for mode %u: %u (%s)", mode, status, desc);
        if (mode == 2026 && decrpm_is_set_or_reset(status)) {
            return KEYCODE_QUERY_REPLY_BIT | TFLAG_SYNC_CSI;
        }
        return KEY_IGNORE;
    }

    if (final == 'u' && intermediate == 0 && nparams == 1) {
        // Kitty keyboard protocol flags (CSI ? flags u)
        unsigned int flags = csi->params[0][0];
        LOG_DEBUG("query reply for kittykbd flags: 0x%x", flags);
        // Interpret reply with any flags to mean "supported"
        return KEYCODE_QUERY_REPLY_BIT | TFLAG_KITTY_KEYBOARD;
    }

ignore:
    // TODO: Log (escaped) sequence string
    LOG_DEBUG("unknown CSI with '?' parameter prefix");
    return KEY_IGNORE;
}

static ssize_t parse_csi(const char *buf, size_t len, size_t i, KeyCode *k)
{
    ControlParams csi = {.nparams = 0};
    bool maybe_query_reply = (i < len && buf[i] == '?');
    i = parse_csi_params(buf, len, i + (maybe_query_reply ? 1 : 0), &csi);

    if (unlikely(csi.final_byte == 0)) {
        BUG_ON(i < len);
        return -1;
    }
    if (unlikely(csi.unhandled_bytes)) {
        goto ignore;
    }

    if (maybe_query_reply) {
        *k = parse_csi_query_reply(&csi);
        return i;
    }

    if (unlikely(csi.nr_intermediate)) {
        goto ignore;
    }

    /*
     * This handles the basic CSI u ("fixterms") encoding and also the
     * extended kitty keyboard encoding.
     *
     * - https://www.leonerd.org.uk/hacks/fixterms/
     * - https://sw.kovidgoyal.net/kitty/keyboard-protocol/
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
            mods = decode_extended_modifiers(csi.params[1][0]);
            if (unlikely(mods == 0)) {
                goto ignore;
            }
            // Fallthrough
        case 1:
            *k = normalize_extended_keycode(mods, key);
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
            if (unlikely(mods == 0)) {
                goto ignore;
            }
            key = csi.params[2][0];
            *k = normalize_extended_keycode(mods, key);
            return i;
        case 2:
            mods = decode_modifiers(csi.params[1][0]);
            if (unlikely(mods == 0)) {
                goto ignore;
            }
            // Fallthrough
        case 1:
            key = decode_key_from_param(csi.params[0][0]);
            if (key == 0) {
                if (csi.params[0][0] == 200 && mods == 0) {
                    *k = KEY_BRACKETED_PASTE;
                    return i;
                }
                goto ignore;
            }
            *k = mods | key;
            return i;
        }
        goto ignore;
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
        if (unlikely(mods == 0 || csi.params[0][0] != 1)) {
            goto ignore;
        }
        // Fallthrough
    case 0:
        key = decode_key_from_final_byte(csi.final_byte);
        if (unlikely(key == 0)) {
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
    size_t pos = 0;

    while (i < len) {
        const unsigned char ch = buf[i++];
        if (unlikely(ch < 0x20)) {
            switch (ch) {
            case 0x18: // CAN
            case 0x1A: // SUB
                goto ignore;
            case 0x1B: // ESC
                i--;
                // Fallthrough
            case 0x07: // BEL
                goto complete;
            }
            continue;
        }
        // Consume 0x20..0xFF (UTF-8 allowed)
        if (likely(pos < sizeof(data))) {
            data[pos++] = ch;
        }
    }

    // Unterminated sequence (possibly truncated)
    return -1;

complete:
    if (unlikely(pos == 0)) {
        goto ignore;
    }

    const char *note = unlikely(pos >= sizeof(data)) ? " (truncated)" : "";
    char prefix = data[0];
    if (prefix == 'L' || prefix == 'l') {
        const char *type = (prefix == 'l') ? "title" : "icon";
        LOG_DEBUG("window %s%s: %.*s", type, note, (int)pos - 1, data + 1);
    } else {
        LOG_WARNING("unknown OSC string%s: %.*s", note, (int)pos, data);
    }

ignore:
    *k = KEY_IGNORE;
    return i;
}

ssize_t term_parse_sequence(const char *buf, size_t length, KeyCode *k)
{
    if (unlikely(length == 0 || buf[0] != '\033')) {
        return 0;
    } else if (unlikely(length == 1)) {
        return -1;
    }
    switch (buf[1]) {
    case 'O': return parse_ss3(buf, length, 2, k);
    case '[': return parse_csi(buf, length, 2, k);
    case ']': return parse_osc(buf, length, 2, k);
    // String Terminator (see https://vt100.net/emu/dec_ansi_parser#STESC)
    case '\\': *k = KEY_IGNORE; return 2;
    }
    return 0;
}
