#ifndef TERMINAL_KEY_H
#define TERMINAL_KEY_H

#include <stdbool.h>
#include <stdint.h>
#include "../util/macros.h"

enum {
    KEY_ENTER = '\n',

    // This is the maximum Unicode codepoint allowed by RFC 3629.
    // When stored in a 32-bit integer, it only requires the first
    // 21 low-order bits, leaving 11 high-order bits available to
    // be used as bit flags.
    KEY_UNICODE_MAX = 0x010FFFF,

    // In addition to the 11 unused, high-order bits, there are also
    // some unused values in the range from KEY_UNICODE_MAX + 1 to
    // (1 << 21) - 1, which can be used to represent special keys.
    KEY_SPECIAL_MIN = KEY_UNICODE_MAX + 1,

    // Note: these must be kept in sync with the array of names in key.c
    KEY_INSERT = KEY_SPECIAL_MIN,
    KEY_DELETE,
    KEY_UP,
    KEY_DOWN,
    KEY_RIGHT,
    KEY_LEFT,
    KEY_PAGE_DOWN,
    KEY_END,
    KEY_PAGE_UP,
    KEY_HOME,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,

    KEY_SPECIAL_MAX = KEY_F12,
    NR_SPECIAL_KEYS = KEY_SPECIAL_MAX - KEY_SPECIAL_MIN + 1,

    // Modifier bit flags (as described above)
    MOD_CTRL  = 0x1000000,
    MOD_META  = 0x2000000,
    MOD_SHIFT = 0x4000000,
    MOD_MASK  = MOD_CTRL | MOD_META | MOD_SHIFT,

    KEY_PASTE = 0x8000000,
};

typedef uint32_t KeyCode;

static inline KeyCode keycode_get_key(KeyCode k)
{
    return k & ~MOD_MASK;
}

static inline KeyCode keycode_get_modifiers(KeyCode k)
{
    return k & MOD_MASK;
}

static inline KeyCode keycode_normalize(KeyCode k)
{
    switch (k) {
    case '\t': return k;
    case '\r': return KEY_ENTER;
    case 0x7F: return MOD_CTRL | '?';
    }
    if (k < 0x20) {
        return MOD_CTRL | k | 0x40;
    }
    return k;
}

#define CTRL(x) (MOD_CTRL | (x))

bool parse_key(KeyCode *key, const char *str);
const char *key_to_string(KeyCode key) RETURNS_NONNULL;

#endif
