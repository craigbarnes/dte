#include "kitty.h"
#include "util/ascii.h"
#include "util/base64.h"
#include "util/macros.h"

/*
 Kitty's extended key encoding scheme is based on a quirky base85
 alphabet and a set of key mappings that are very unamenable to
 algorithmic translation. Since most of the useful keys are encoded
 in a single digit, we may as well just map the bytes directly to
 KeyCodes, instead of doing a base85 decode followed by yet another
 table lookup for translation.

 See: https://sw.kovidgoyal.net/kitty/key-encoding.html
*/
static const KeyCode kitty_1digit_keys[] = {
    ['0'] = KEY_TAB,
    ['1'] = MOD_CTRL | '?', // Backspace
    ['2'] = KEY_INSERT,
    ['3'] = KEY_DELETE,
    ['4'] = KEY_RIGHT,
    ['5'] = KEY_LEFT,
    ['6'] = KEY_DOWN,
    ['7'] = KEY_UP,
    ['8'] = KEY_PAGE_UP,
    ['9'] = KEY_PAGE_DOWN,
    ['-'] = KEY_END,
    ['.'] = KEY_HOME,
    ['/'] = KEY_F1,
    ['*'] = KEY_F2,
    ['?'] = KEY_F3,
    ['&'] = KEY_F4,
    ['<'] = KEY_F5,
    ['>'] = KEY_F6,
    ['('] = KEY_F7,
    [')'] = KEY_F8,
    ['['] = KEY_F9,
    [']'] = KEY_F10,
    ['{'] = KEY_F11,
    ['}'] = KEY_F12,
    ['@'] = KEY_IGNORE, // F13
    ['%'] = KEY_IGNORE, // F14
    ['$'] = KEY_IGNORE, // F15
    ['#'] = KEY_IGNORE, // F16
    ['!'] = KEY_IGNORE, // Pause
    ['+'] = KEY_IGNORE, // Scroll Lock
    [':'] = KEY_IGNORE, // Caps Lock
    ['='] = KEY_IGNORE, // Num Lock
    ['^'] = KEY_IGNORE, // Print Screen

    ['A'] = KEY_SPACE,
    ['B'] = '\'', ['C'] = ',', ['D'] = '-', ['E'] = '.', ['F'] = '/',

    ['G'] = '0', ['H'] = '1', ['I'] = '2', ['J'] = '3', ['K'] = '4',
    ['L'] = '5', ['M'] = '6', ['N'] = '7', ['O'] = '8', ['P'] = '9',

    ['Q'] = ';', ['R'] = '=',

    ['S'] = 'a', ['T'] = 'b', ['U'] = 'c', ['V'] = 'd', ['W'] = 'e',
    ['X'] = 'f', ['Y'] = 'g', ['Z'] = 'h',

    ['a'] = 'i', ['b'] = 'j', ['c'] = 'k', ['d'] = 'l', ['e'] = 'm',
    ['f'] = 'n', ['g'] = 'o', ['h'] = 'p', ['i'] = 'q', ['j'] = 'r',
    ['k'] = 's', ['l'] = 't', ['m'] = 'u', ['n'] = 'v', ['o'] = 'w',
    ['p'] = 'x', ['q'] = 'y', ['r'] = 'z',

    ['s'] = '[', ['t'] = '\\', ['u'] = ']', ['v'] = '`',

    ['w'] = KEY_IGNORE, // "World 1"
    ['x'] = KEY_IGNORE, // "World 2"
    ['y'] = MOD_CTRL | '[', // Escape
    ['z'] = KEY_ENTER,
};

// https://sw.kovidgoyal.net/kitty/protocol-extensions.html#keyboard-handling
ssize_t kitty_parse_full_mode_key(const char *buf, size_t length, size_t i, KeyCode *k)
{
    if (unlikely(i >= length)) {
        return -1;
    }

    // Note: "\e_K" has already been parsed by parse_apc()
    bool is_key_release;
    switch (buf[i++]) {
    case 'p': // Press
    case 't': // Repeat
        is_key_release = false;
        break;
    case 'r': // Release
        is_key_release = true;
        break;
    default:
        return 0;
    }

    if (unlikely(i >= length)) {
        return -1;
    }
    KeyCode mods = base64_decode(buf[i++]);
    if (unlikely(mods > 15)) {
        return 0;
    }

    if (unlikely(i >= length)) {
        return -1;
    }

    unsigned char c1 = buf[i++];
    KeyCode key;
    if (c1 == 'B') {
        // Handle (possibly 2-digit) keys starting with 'B'
        if (unlikely(i >= length)) {
            return -1;
        }
        unsigned char c2 = buf[i++];
        switch (c2) {
        // 1-digit 'B' is apostrophe key (\033 is probably start of ST)
        case '\033':
            key = '\'';
            goto st_slash;
        // Left/right Shift/Ctrl/Alt/Super
        case 'a': case 'b': case 'c': case 'd':
        case 'e': case 'f': case 'g': case 'h':
        // F17-F25
        case 'A': case 'B': case 'C': case 'D':
        case 'E': case 'F': case 'G': case 'H':
        case 'I':
            key = KEY_IGNORE;
            break;
        // Keypad 0-9
        case 'J': case 'K': case 'L': case 'M':
        case 'N': case 'O': case 'P': case 'Q':
        case 'R': case 'S':
            key = '0' + (c2 - 'J');
            break;
        // Keypad other
        case 'T': key = '.'; break;
        case 'U': key = '/'; break;
        case 'V': key = '*'; break;
        case 'W': key = '-'; break;
        case 'X': key = '+'; break;
        case 'Y': key = KEY_ENTER; break;
        case 'Z': key = '='; break;
        default:
            return 0;
        }
    } else {
        // Look up 1-digit keys
        if (unlikely(c1 >= ARRAY_COUNT(kitty_1digit_keys))) {
            return 0;
        }
        key = kitty_1digit_keys[c1];
        if (unlikely(key == 0)) {
            return 0;
        }
    }

    // Check string terminator (ST)
    if (unlikely(i >= length)) {
        return -1;
    } else if (buf[i++] != '\033') {
        return 0;
    }
    st_slash:
    if (unlikely(i >= length)) {
        return -1;
    } else if (buf[i++] != '\\') {
        return 0;
    }

    if (is_key_release || key == KEY_IGNORE || mods > 7) {
        *k = KEY_IGNORE;
        return i;
    }

    mods <<= MOD_OFFSET;
    if (unlikely(
        mods & (MOD_CTRL | MOD_SHIFT)
        && key < 127
        && ascii_isalpha(key)
    )) {
        // Remove Shift modifier for Ctrl+Shift+[A-Z] combos; for the
        // same reason as noted in normalize_modified_other_key()
        mods &= ~MOD_SHIFT;
        key = ascii_toupper(key);
    }

    *k = mods | key;
    return i;
}
