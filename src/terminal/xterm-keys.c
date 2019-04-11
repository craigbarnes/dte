// Escape sequence parser for xterm function keys.
// Copyright 2018-2019 Craig Barnes.
// SPDX-License-Identifier: GPL-2.0-only
// See also: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html

#include <stdint.h>
#include "xterm.h"
#include "../util/ascii.h"
#include "../util/unicode.h"

static KeyCode decode_modifiers(uint32_t n)
{
    switch (n) {
    case 2: return MOD_SHIFT;
    case 3: return MOD_META;
    case 4: return MOD_SHIFT | MOD_META;
    case 5: return MOD_CTRL;
    case 6: return MOD_SHIFT | MOD_CTRL;
    case 7: return MOD_META | MOD_CTRL;
    case 8: return MOD_SHIFT | MOD_META | MOD_CTRL;
    }
    return 0;
}

static KeyCode decode_special_key(uint32_t code)
{
    switch (code) {
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
    }
    return 0;
}

static ssize_t parse_ss3(const char *buf, size_t length, size_t i, KeyCode *k)
{
    if (i >= length) {
        return -1;
    }
    const char ch = buf[i++];
    switch (ch) {
    case 'A': // Up
    case 'B': // Down
    case 'C': // Right
    case 'D': // Left
    case 'F': // End
    case 'H': // Home
        *k = KEY_UP + (ch - 'A');
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
        *k = ch;
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
    case 'y': case 'I': case 'M':
        *k = ch - 64;
        return i;
    }
    return 0;
}

static ssize_t parse_csi_num(const char *buf, size_t len, size_t i, KeyCode *k)
{
    uint32_t params[4] = {0, 0, 0, 0};
    size_t nparams = 0;
    uint8_t final_byte = 0;

    uint32_t num = 0;
    size_t digits = 0;
    while (i < len) {
        const char ch = buf[i++];
        switch (ch) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            num = (num * 10) + (ch - '0');
            if (num > UNICODE_MAX_VALID_CODEPOINT) {
                return 0;
            }
            digits++;
            continue;
        case ';':
            params[nparams++] = num;
            if (nparams > 2) {
                return 0;
            }
            num = 0;
            digits = 0;
            continue;
        case 'A': case 'B': case 'C': case 'D': case 'F':
        case 'H': case 'P': case 'Q': case 'R': case 'S':
        case 'u': case '~':
            final_byte = ch;
            if (digits > 0) {
                params[nparams++] = num;
            }
            goto exit_loop;
        }
        return 0;
    }
exit_loop:

    if (final_byte == 0) {
        return (i >= len) ? -1 : 0;
    }

    KeyCode mods = 0;
    KeyCode key;

    switch (nparams) {
    case 3:
        if (params[0] == 27 && final_byte == '~') {
            mods = decode_modifiers(params[1]);
            if (mods == 0) {
                return 0;
            }
            *k = mods | params[2];
            return i;
        }
        return 0;
    case 2:
        mods = decode_modifiers(params[1]);
        if (mods == 0) {
            return 0;
        }
        switch (final_byte) {
        case '~':
            goto check_first_param_is_special_key;
        case 'u':
            key = params[0];
            goto set_k_and_return_i;
        case 'A': // Up
        case 'B': // Down
        case 'C': // Right
        case 'D': // Left
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
    }
    return 0;

check_first_param_is_special_key:
    key = decode_special_key(params[0]);
    if (key == 0) {
        return 0;
    }
    goto set_k_and_return_i;
check_first_param_is_1:
    if (params[0] != 1) {
        return 0;
    }
set_k_and_return_i:
    *k = mods | key;
    return i;
}

static ssize_t parse_csi(const char *buf, size_t length, size_t i, KeyCode *k)
{
    if (i >= length) {
        return -1;
    }
    char ch = buf[i++];
    switch (ch) {
    case 'A': // Up
    case 'B': // Down
    case 'C': // Right
    case 'D': // Left
    case 'F': // End
    case 'H': // Home
        *k = KEY_UP + (ch - 'A');
        return i;
    case 'a': // Shift+Up (rxvt)
    case 'b': // Shift+Down (rxvt)
    case 'c': // Shift+Right (rxvt)
    case 'd': // Shift+Left (rxvt)
        *k = MOD_SHIFT | (KEY_UP + (ch - 'a'));
        return i;
    case 'L':
        *k = KEY_INSERT;
        return i;
    case 'Z':
        *k = MOD_SHIFT | '\t';
        return i;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        return parse_csi_num(buf, length, i - 1, k);
    case '[':
        if (i >= length) {
            return -1;
        }
        switch (ch = buf[i++]) {
        // Linux console keys
        case 'A': // F1
        case 'B': // F2
        case 'C': // F3
        case 'D': // F4
        case 'E': // F5
            *k = KEY_F1 + (ch - 'A');
            return i;
        }
        return 0;
    }
    return 0;
}

ssize_t xterm_parse_key(const char *buf, size_t length, KeyCode *k)
{
    if (length == 0 || buf[0] != '\033') {
        return 0;
    } else if (length == 1) {
        return -1;
    }
    switch (buf[1]) {
    case 'O': return parse_ss3(buf, length, 2, k);
    case '[': return parse_csi(buf, length, 2, k);
    }
    return 0;
}
