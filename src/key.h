#ifndef KEY_H
#define KEY_H

#include <stdbool.h>
#include <stdint.h>

enum {
    KEY_ENTER = '\n',

    // This is the maximum Unicode codepoint allowed by RFC 3629.
    // When stored in a 32-bit integer, it only requires the first
    // 21 low-order bits, leaving 11 high-order bits available to
    // be used as bit flags.
    KEY_UNICODE_MAX = UINT32_C(0x010ffff),

    // In addition to the 11 unused, high-order bits, there are also
    // some unused values in the range from KEY_UNICODE_MAX + 1 to
    // (1 << 21) - 1, which can be used to represent special keys.
    KEY_SPECIAL_MIN = KEY_UNICODE_MAX + 1,

    // Note: these must be kept in sync with the array of names in key.c
    KEY_INSERT = KEY_SPECIAL_MIN,
    KEY_DELETE,
    KEY_HOME,
    KEY_END,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_UP,
    KEY_DOWN,
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

    // Modifier keys stored as bit flags (as described above).
    MOD_CTRL  = UINT32_C(0x1000000), // 1 << 24
    MOD_META  = UINT32_C(0x2000000), // 1 << 25
    MOD_SHIFT = UINT32_C(0x4000000), // 1 << 26
    MOD_MASK  = UINT32_C(0x7000000), // MOD_CTRL | MOD_META | MOD_SHIFT

    KEY_PASTE = UINT32_C(0x8000000), // 1 << 27 (not a key)
};

typedef uint32_t Key;

#define CTRL(x) (MOD_CTRL | (x))

bool parse_key(Key *key, const char *str);
char *key_to_string(Key key);
bool key_to_ctrl(Key key, unsigned char *byte);

#endif
