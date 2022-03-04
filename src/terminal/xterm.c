// Escape sequence parser for xterm function keys.
// Copyright 2018-2022 Craig Barnes.
// SPDX-License-Identifier: GPL-2.0-only
// See also: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html

#include <stdint.h>
#include "xterm.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/string-view.h"
#include "util/unicode.h"

typedef enum {
    BYTE_CONTROL,       // 0x00..0x1F
    BYTE_INTERMEDIATE,  // 0x20..0x2F
    BYTE_PARAMETER,     // 0x30..0x3F
    BYTE_FINAL,         // 0x40..0x6F
    BYTE_FINAL_PRIVATE, // 0x70..0x7E
    BYTE_DELETE,        // 0x7F
    BYTE_OTHER,
} ByteType;

#define ENTRY(x) [(x - 64)]

static const KeyCode final_byte_keys[] = {
    ENTRY('A') = KEY_UP,
    ENTRY('B') = KEY_DOWN,
    ENTRY('C') = KEY_RIGHT,
    ENTRY('D') = KEY_LEFT,
    ENTRY('E') = KEY_BEGIN, // (keypad '5')
    ENTRY('F') = KEY_END,
    ENTRY('H') = KEY_HOME,
    ENTRY('L') = KEY_INSERT,
    ENTRY('M') = KEY_ENTER,
    ENTRY('P') = KEY_F1,
    ENTRY('Q') = KEY_F2,
    ENTRY('R') = KEY_F3,
    ENTRY('S') = KEY_F4,
};

static const KeyCode param_keys[] = {
    [1] = KEY_HOME,
    [2] = KEY_INSERT,
    [3] = KEY_DELETE,
    [4] = KEY_END,
    [5] = KEY_PAGE_UP,
    [6] = KEY_PAGE_DOWN,
    [7] = KEY_HOME,
    [8] = KEY_END,
    [11] = KEY_F1,
    [12] = KEY_F2,
    [13] = KEY_F3,
    [14] = KEY_F4,
    [15] = KEY_F5,
    [17] = KEY_F6,
    [18] = KEY_F7,
    [19] = KEY_F8,
    [20] = KEY_F9,
    [21] = KEY_F10,
    [23] = KEY_F11,
    [24] = KEY_F12,
    [25] = KEY_F13,
    [26] = KEY_F14,
    [28] = KEY_F15,
    [29] = KEY_F16,
    [31] = KEY_F17,
    [32] = KEY_F18,
    [33] = KEY_F19,
    [34] = KEY_F20,
};

static KeyCode decode_kitty_special_key(uint32_t n)
{
    switch (n) {
    // case 57359: return KEY_SCROLL_LOCK;
    // case 57361: return KEY_PRINT_SCREEN;
    // case 57362: return KEY_PAUSE;
    // case 57363: return KEY_MENU;
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

    static_assert(MOD_MASK >> MOD_OFFSET == 7);

    // Decode Meta (bit 4) and/or Alt (bit 2) as just Meta
    KeyCode mods = (n & 7) | ((n & 8) >> 2);
    BUG_ON(mods > 7);

    return mods << MOD_OFFSET;
}

static KeyCode decode_key_from_param(uint32_t param)
{
    return (param < ARRAY_COUNT(param_keys)) ? param_keys[param] : 0;
}

static KeyCode decode_key_from_final_byte(uint8_t byte)
{
    byte -= 64;
    return (byte < ARRAY_COUNT(final_byte_keys)) ? final_byte_keys[byte] : 0;
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
        return i;
    }

    switch (ch) {
    case 'X':
        *k = '=';
        return i;
    case ' ':
        *k = KEY_SPACE;
        return i;
    case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o':
    case 'p': case 'q': case 'r':
    case 's': case 't': case 'u':
    case 'v': case 'w': case 'x':
    case 'y': case 'I':
        *k = ch - 64;
        return i;
    }
    *k = KEY_IGNORE;
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
    bool unhandled_bytes = false;

    while (i < len) {
        const char ch = buf[i++];
        switch (ch) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            num = (num * 10) + (ch - '0');
            if (unlikely(num > UNICODE_MAX_VALID_CODEPOINT)) {
                unhandled_bytes = true;
            }
            digits++;
            continue;
        case ';':
            if (unlikely(nparams >= ARRAY_COUNT(csi->params))) {
                unhandled_bytes = true;
                continue;
            }
            csi->nsub[nparams] = sub + 1;
            csi->params[nparams++][sub] = num;
            num = 0;
            digits = 0;
            sub = 0;
            continue;
        case ':':
            if (unlikely(sub >= ARRAY_COUNT(csi->params[0]))) {
                unhandled_bytes = true;
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
            if (unlikely(nr_intermediate >= ARRAY_COUNT(csi->intermediate))) {
                unhandled_bytes = true;
            } else {
                csi->intermediate[nr_intermediate++] = ch;
            }
            continue;
        case BYTE_FINAL:
        case BYTE_FINAL_PRIVATE:
            csi->final_byte = ch;
            if (digits > 0) {
                if (unlikely(
                    nparams >= ARRAY_COUNT(csi->params)
                    || sub >= ARRAY_COUNT(csi->params[0])
                )) {
                    unhandled_bytes = true;
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
            unhandled_bytes = true;
            continue;
        case BYTE_CONTROL:
            switch (ch) {
            case 0x1B: // ESC
                // Don't consume ESC; it's the start of another sequence
                i--;
                // Fallthrough
            case 0x18: // CAN
            case 0x1A: // SUB
                csi->final_byte = ch;
                unhandled_bytes = true;
                goto exit_loop;
            }
            // Fallthrough
        case BYTE_DELETE:
            continue;
        case BYTE_OTHER:
            unhandled_bytes = true;
            continue;
        }

        BUG("unhandled byte type");
        unhandled_bytes = true;
    }

exit_loop:
    csi->nparams = nparams;
    csi->nr_intermediate = nr_intermediate;
    csi->have_subparams = have_subparams;
    csi->unhandled_bytes = unhandled_bytes;
    return i;
}

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
    static_assert(ARRAY_COUNT(csi.nsub) == 4);
    static_assert(ARRAY_COUNT(csi.nsub) == ARRAY_COUNT(csi.params[0]));
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
    // TODO: support parsing '?' as param prefix?
    BUG_ON(!csi.unhandled_bytes);
}

static ssize_t parse_csi(const char *buf, size_t len, size_t i, KeyCode *k)
{
    ControlParams csi = {.nparams = 0};
    i = parse_csi_params(buf, len, i, &csi);

    if (unlikely(csi.final_byte == 0)) {
        BUG_ON(i < len);
        return -1;
    }
    if (unlikely(csi.unhandled_bytes || csi.nr_intermediate)) {
        goto ignore;
    }

    KeyCode key, mods = 0;
    if (csi.final_byte == 'u') {
        if (csi.nsub[0] > 3 || csi.nsub[1] > 1) {
            goto ignore;
        }
        key = csi.params[0][csi.nsub[0] == 3 ? 2 : 0];
        switch (csi.nparams) {
        case 2:
            goto decode_mods_and_normalize;
        case 1:
            goto normalize;
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
            key = csi.params[2][0];
            goto decode_mods_and_normalize;
        case 2:
            mods = decode_modifiers(csi.params[1][0]);
            if (unlikely(mods == 0)) {
                goto ignore;
            }
            // Fallthrough
        case 1:
            key = decode_key_from_param(csi.params[0][0]);
            if (unlikely(key == 0)) {
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

decode_mods_and_normalize:
    mods = decode_modifiers(csi.params[1][0]);
    if (unlikely(mods == 0)) {
        goto ignore;
    }
normalize:
    *k = normalize_extended_keycode(mods, key);
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
    if (pos > 0) {
        const char prefix = data[0];
        if (prefix == 'L' || prefix == 'l') {
            const char *note = (pos >= sizeof(data)) ? " (truncated)" : "";
            const char *type = (prefix == 'l') ? "title" : "icon";
            DEBUG_LOG("window %s%s: %.*s", type, note, (int)pos - 1, data + 1);
        }
    }

ignore:
    *k = KEY_IGNORE;
    return i;
}

ssize_t xterm_parse_key(const char *buf, size_t length, KeyCode *k)
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
    case '\\': *k = KEY_IGNORE; return 2;
    }
    return 0;
}
