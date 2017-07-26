#ifndef KEY_H
#define KEY_H

#include "libc.h"

enum {
	KEY_ENTER = '\n',
	KEY_UNICODE_MAX  = 0x010ffff,

	KEY_SPECIAL_MIN = 0x0110000,

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

	MOD_CTRL  = 0x1000000U,
	MOD_META  = 0x2000000U,
	MOD_SHIFT = 0x4000000U,
	MOD_MASK  = 0x7000000U,

	KEY_PASTE = 0x8000000U, // not a key
};

#define CTRL(x) (MOD_CTRL | (x))

bool parse_key(int *key, const char *str);
char *key_to_string(int key);
bool key_to_ctrl(int key, unsigned char *byte);

#endif
