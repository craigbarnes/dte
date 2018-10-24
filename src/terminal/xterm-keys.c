// Escape sequence parser for xterm function keys.
// Copyright 2018 Craig Barnes.
// SPDX-License-Identifier: GPL-2.0-only
// See also: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html

#include "xterm.h"

// These values are used in xterm escape sequences to indicate
// modifier+key combinations.
// See also: user_caps(5)
static KeyCode mod_enum_to_mod_mask(char mod_enum)
{
    switch (mod_enum) {
    case '2': return MOD_SHIFT;
    case '3': return MOD_META;
    case '4': return MOD_SHIFT | MOD_META;
    case '5': return MOD_CTRL;
    case '6': return MOD_SHIFT | MOD_CTRL;
    case '7': return MOD_META | MOD_CTRL;
    case '8': return MOD_SHIFT | MOD_META | MOD_CTRL;
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
    case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u':
    case 'v': case 'w': case 'x': case 'y':
    case 'z': case 'I': case 'M':
        *k = ch - 64;
        return i;
    }
    return 0;
}

static ssize_t parse_csi1(const char *buf, size_t length, size_t i, KeyCode *k)
{
    if (i >= length) {
        return -1;
    }
    KeyCode tmp = mod_enum_to_mod_mask(buf[i++]);
    if (tmp == 0) {
        return 0;
    } else if (i >= length) {
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
        tmp |= KEY_UP + (ch - 'A');
        goto match;
    case 'P': // F1
    case 'Q': // F2
    case 'R': // F3
    case 'S': // F4
        tmp |= KEY_F1 + (ch - 'P');
        goto match;
    }
    return 0;
match:
    *k = tmp;
    return i;
}

static ssize_t parse_csi(const char *buf, size_t length, size_t i, KeyCode *k)
{
    if (i >= length) {
        return -1;
    }
    KeyCode tmp;
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
    case 'L': *k = KEY_INSERT; return i;
    case 'Z': *k = MOD_SHIFT | '\t'; return i;
    case '3': tmp = KEY_DELETE; goto check_delim;
    case '4': tmp = KEY_END; goto check_trailing_tilde;
    case '5': tmp = KEY_PAGE_UP; goto check_delim;
    case '6': tmp = KEY_PAGE_DOWN; goto check_delim;
    case '1':
        if (i >= length) return -1;
        switch (ch = buf[i++]) {
        case '1': // F1
        case '2': // F2
        case '3': // F3
        case '4': // F4
        case '5': // F5
            tmp = KEY_F1 + (ch - '1');
            goto check_delim;
        case '7': // F6
        case '8': // F7
        case '9': // F8
            tmp = KEY_F6 + (ch - '7');
            goto check_delim;
        case ';': return parse_csi1(buf, length, i, k);
        case '~': *k = KEY_HOME; return i;
        }
        return 0;
    case '2':
        if (i >= length) return -1;
        switch (buf[i++]) {
        case '0': tmp = KEY_F9; goto check_delim;
        case '1': tmp = KEY_F10; goto check_delim;
        case '3': tmp = KEY_F11; goto check_delim;
        case '4': tmp = KEY_F12; goto check_delim;
        case ';': tmp = KEY_INSERT; goto check_modifiers;
        case '~': *k = KEY_INSERT; return i;
        }
        return 0;
    case '[':
        if (i >= length) return -1;
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
check_delim:
    if (i >= length) return -1;
    switch (buf[i++]) {
    case ';':
        goto check_modifiers;
    case '~':
        *k = tmp;
        return i;
    }
    return 0;
check_modifiers:
    if (i >= length) {
        return -1;
    }
    const KeyCode mods = mod_enum_to_mod_mask(buf[i++]);
    if (mods == 0) {
        return 0;
    }
    tmp |= mods;
check_trailing_tilde:
    if (i >= length) {
        return -1;
    } else if (buf[i++] == '~') {
        *k = tmp;
        return i;
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
