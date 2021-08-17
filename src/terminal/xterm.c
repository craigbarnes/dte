// Escape sequence parser for xterm function keys.
// Copyright 2018-2020 Craig Barnes.
// SPDX-License-Identifier: GPL-2.0-only
// See also: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html

#include <stdint.h>
#include "xterm.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/macros.h"
#include "util/unicode.h"

typedef enum {
    BYTE_INVALID,
    BYTE_INTERMEDIATE, // 32..47
    BYTE_PARAMETER, // 48..63
    BYTE_FINAL, // 64..111
    BYTE_FINAL_PRIVATE, // 112..126
} ByteType;

static const KeyCode special_keys[] = {
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

static KeyCode decode_modifiers(uint32_t n)
{
    n--;
    if (unlikely(n > 15)) {
        return 0;
    }

    // Decode Meta (bit 4) and/or Alt (bit 2) as just Meta
    KeyCode mods = (n & 7) | ((n & 8) >> 2);
    BUG_ON(mods > 7);

    return mods << MOD_OFFSET;
}

static KeyCode decode_special_key(uint32_t n)
{
    return (n >= ARRAY_COUNT(special_keys)) ? 0 : special_keys[n];
}

// Fix quirky key codes sent when "modifyOtherKeys" is enabled
static KeyCode normalize_modified_other_key(KeyCode mods, KeyCode key)
{
    if (key > 0x20 && key < 0x80) {
        // The Shift modifier is never appropriate with the
        // printable ASCII range, since pressing Shift causes
        // the base key itself to change (i.e. "r" becomes "R',
        // "." becomes ">", etc.)
        mods &= ~MOD_SHIFT;

        static_assert(MOD_CTRL >> 21 == 0x20);
        static_assert(ASCII_LOWER << 1 == 0x20);

        // Presence of the Ctrl modifier should always cause letters
        // to be uppercase. This assumption is too ingrained and
        // causes too much breakage if not enforced.
        // Note: the code below is a branchless equivalent of:
        // `if (mods & MOD_CTRL) key = ascii_toupper(key);`.
        key -= ((mods & MOD_CTRL) >> 21) & ((ascii_table[key] & ASCII_LOWER) << 1);
    }

    return mods | keycode_normalize(key);
}

static ssize_t parse_ss3(const char *buf, size_t length, size_t i, KeyCode *k)
{
    if (unlikely(i >= length)) {
        return -1;
    }
    const char ch = buf[i++];
    switch (ch) {
    case 'A': // Up
    case 'B': // Down
    case 'C': // Right
    case 'D': // Left
    case 'E': // Begin (keypad '5')
    case 'F': // End
    case 'H': // Home
        *k = KEY_UP + (ch - 'A');
        return i;
    case 'M':
        *k = KEY_ENTER;
        return i;
    case 'P': // F1
    case 'Q': // F2
    case 'R': // F3
    case 'S': // F4
        *k = KEY_F1 + (ch - 'P');
        return i;
    case 'X':
        *k = '=';
        return i;
    case ' ':
        *k = KEY_SPACE;
        return i;
    case 'a': // Ctrl+Up (rxvt)
    case 'b': // Ctrl+Down (rxvt)
    case 'c': // Ctrl+Right (rxvt)
    case 'd': // Ctrl+Left (rxvt)
        *k = MOD_CTRL | (KEY_UP + (ch - 'a'));
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
    return 0;
}

static ByteType get_byte_type(unsigned char byte)
{
    enum {
        I = BYTE_INTERMEDIATE,
        P = BYTE_PARAMETER,
        F = BYTE_FINAL,
        f = BYTE_FINAL_PRIVATE,
        x = BYTE_INVALID
    };

    // ECMA 48 divides bytes ("bit combinations") into 16 rows of 16 columns.
    // The byte classifications mostly fall into their own rows:
    static const uint8_t rows[16] = {
        x, x, I, P, F, F, F, f,
        x, x, x, x, x, x, x, x
    };

    // ... with the single exception being byte 127 (DEL), which falls into
    // row 7, but isn't a final byte like the others in that row:
    if (unlikely(byte == 127)) {
        return BYTE_INVALID;
    }

    return rows[(byte >> 4) & 0xF];
}

static ssize_t parse_csi(const char *buf, size_t len, size_t i, KeyCode *k)
{
    uint32_t params[4] = {0};
    size_t nparams = 0;
    uint8_t intermediate[4] = {0};
    size_t nr_intermediate = 0;
    uint8_t final_byte = 0;
    bool unhandled_bytes = false;
    uint32_t num = 0;
    size_t digits = 0;

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
            if (unlikely(nparams >= ARRAY_COUNT(params))) {
                unhandled_bytes = true;
                continue;
            }
            params[nparams++] = num;
            num = 0;
            digits = 0;
            continue;
        case ':':
            // TODO: handle sub-params
            unhandled_bytes = true;
            continue;
        }

        switch (get_byte_type(ch)) {
        case BYTE_INTERMEDIATE:
            if (unlikely(nr_intermediate >= ARRAY_COUNT(intermediate))) {
                unhandled_bytes = true;
            } else {
                intermediate[nr_intermediate++] = ch;
            }
            continue;
        case BYTE_FINAL:
        case BYTE_FINAL_PRIVATE:
            final_byte = ch;
            if (digits > 0) {
                if (unlikely(nparams >= ARRAY_COUNT(params))) {
                    unhandled_bytes = true;
                } else {
                    params[nparams++] = num;
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
        case BYTE_INVALID:
            return 0;
        }

        BUG("unhandled byte type");
        return 0;
    }

exit_loop:

    if (unlikely(final_byte == 0)) {
        return (i >= len) ? -1 : 0;
    }

    if (unlikely(unhandled_bytes || nr_intermediate)) {
        *k = KEY_IGNORE;
        return i;
    }

    KeyCode mods = 0;
    KeyCode key;

    switch (nparams) {
    case 3:
        if (params[0] == 27 && final_byte == '~') {
            mods = decode_modifiers(params[1]);
            if (unlikely(mods == 0)) {
                return 0;
            }
            *k = normalize_modified_other_key(mods, params[2]);
            return i;
        }
        return 0;
    case 2:
        mods = decode_modifiers(params[1]);
        if (unlikely(mods == 0)) {
            return 0;
        }
        switch (final_byte) {
        case '~':
            goto check_first_param_is_special_key;
        case 'u':
            *k = normalize_modified_other_key(mods, params[0]);
            return i;
        case 'A': // Up
        case 'B': // Down
        case 'C': // Right
        case 'D': // Left
        case 'E': // Begin (keypad '5')
        case 'F': // End
        case 'H': // Home
            key = KEY_UP + (final_byte - 'A');
            goto check_first_param_is_1;
        case 'P': // F1
        case 'Q': // F2
        case 'R': // F3
        case 'S': // F4
            key = KEY_F1 + (final_byte - 'P');
            goto check_first_param_is_1;
        }
        return 0;
    case 1:
        if (final_byte == '~') {
            goto check_first_param_is_special_key;
        }
        return 0;
    case 0:
        switch (final_byte) {
        case 'A': // Up
        case 'B': // Down
        case 'C': // Right
        case 'D': // Left
        case 'E': // Begin (keypad '5')
        case 'F': // End
        case 'H': // Home
            *k = KEY_UP + (final_byte - 'A');
            return i;
        case 'P': // F1
        case 'Q': // F2
        case 'R': // F3
        case 'S': // F4
            *k = KEY_F1 + (final_byte - 'P');
            return i;
        case 'a': // Shift+Up (rxvt)
        case 'b': // Shift+Down (rxvt)
        case 'c': // Shift+Right (rxvt)
        case 'd': // Shift+Left (rxvt)
            *k = MOD_SHIFT | (KEY_UP + (final_byte - 'a'));
            return i;
        case 'L':
            *k = KEY_INSERT;
            return i;
        case 'Z':
            *k = MOD_SHIFT | KEY_TAB;
            return i;
        }
    }
    return 0;

check_first_param_is_special_key:
    key = decode_special_key(params[0]);
    if (unlikely(key == 0)) {
        return 0;
    }
    goto set_k_and_return_i;
check_first_param_is_1:
    if (unlikely(params[0] != 1)) {
        return 0;
    }
set_k_and_return_i:
    *k = mods | key;
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
    }
    return 0;
}
