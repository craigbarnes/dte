#include <string.h>
#include "key.h"
#include "util/array.h"
#include "util/ascii.h"
#include "util/utf8.h"
#include "util/xsnprintf.h"

// Note: these strings must be kept in sync with the enum in key.h
static const char special_names[][8] = {
    "insert",
    "delete",
    "up",
    "down",
    "right",
    "left",
    "begin",
    "end",
    "pgdown",
    "home",
    "pgup",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
    "F13",
    "F14",
    "F15",
    "F16",
    "F17",
    "F18",
    "F19",
    "F20",
};

static const struct {
    char name[8];
    KeyCode key;
} other_keys[] = {
    {"tab", KEY_TAB},
    {"enter", KEY_ENTER},
    {"space", KEY_SPACE},
};

UNITTEST {
    CHECK_STRING_ARRAY(special_names);
    CHECK_STRUCT_ARRAY(other_keys, name);
}

static size_t parse_modifiers(const char *str, KeyCode *modifiersp)
{
    KeyCode modifiers = 0;
    size_t i = 0;

    while (1) {
        KeyCode tmp;
        switch (str[i]) {
        case 'C':
            tmp = MOD_CTRL;
            break;
        case 'M':
            tmp = MOD_META;
            break;
        case 'S':
            tmp = MOD_SHIFT;
            break;
        default:
            goto end;
        }
        if (str[i + 1] != '-' || modifiers & tmp) {
            goto end;
        }
        modifiers |= tmp;
        i += 2;
    }

end:
    *modifiersp = modifiers;
    return i;
}

bool parse_key_string(KeyCode *key, const char *str)
{
    KeyCode modifiers;
    if (str[0] == '^' && str[1] != '\0') {
        modifiers = MOD_CTRL;
        str += 1;
    } else {
        str += parse_modifiers(str, &modifiers);
    }

    const size_t len = strlen(str);
    size_t i = 0;
    KeyCode ch = u_get_char(str, len, &i);
    if (u_is_unicode(ch) && i == len) {
        if (u_is_ascii_upper(ch)) {
            if (modifiers & MOD_CTRL) {
                // Convert C-A to C-a
                ch = ascii_tolower(ch);
            } else if (modifiers & MOD_META) {
                // Convert M-A to M-S-a
                modifiers |= MOD_SHIFT;
                ch = ascii_tolower(ch);
            }
        }
        *key = modifiers | ch;
        return true;
    }

    for (size_t j = 0; j < ARRAYLEN(other_keys); j++) {
        if (ascii_streq_icase(str, other_keys[j].name)) {
            *key = modifiers | other_keys[j].key;
            return true;
        }
    }

    static_assert(ARRAYLEN(special_names) == NR_SPECIAL_KEYS);
    for (i = 0; i < NR_SPECIAL_KEYS; i++) {
        if (ascii_streq_icase(str, special_names[i])) {
            *key = modifiers | (KEY_SPECIAL_MIN + i);
            return true;
        }
    }

    return false;
}

// Writes the string representation of `k` into `buf` (which must
// have at least `KEYCODE_STR_MAX` bytes available) and returns the
// length of the written string.
size_t keycode_to_string(KeyCode k, char *buf)
{
    size_t i = 0;
    if (k & MOD_CTRL) {
        buf[i++] = 'C';
        buf[i++] = '-';
    }
    if (k & MOD_META) {
        buf[i++] = 'M';
        buf[i++] = '-';
    }
    if (k & MOD_SHIFT) {
        buf[i++] = 'S';
        buf[i++] = '-';
    }

    const char *name;
    const KeyCode key = keycode_get_key(k);
    if (u_is_unicode(key)) {
        for (size_t j = 0; j < ARRAYLEN(other_keys); j++) {
            if (key == other_keys[j].key) {
                name = other_keys[j].name;
                goto copy;
            }
        }
        u_set_char(buf, &i, key);
        buf[i] = '\0';
        return i;
    }

    if (key >= KEY_SPECIAL_MIN && key <= KEY_SPECIAL_MAX) {
        name = special_names[key - KEY_SPECIAL_MIN];
        goto copy;
    }

    return xsnprintf(buf, KEYCODE_STR_MAX, "INVALID (0x%08X)", (unsigned int)k);

copy:
    BUG_ON(name[0] == '\0');
    char *end = memccpy(buf + i, name, '\0', KEYCODE_STR_MAX - i);
    BUG_ON(!end);
    BUG_ON(end <= buf);
    return (size_t)(end - buf) - 1;
}
