#include <string.h>
#include "key.h"
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
        if (modifiers == MOD_CTRL) {
            // Normalize
            switch (ch) {
            case 'i':
            case 'I':
                ch = KEY_TAB;
                modifiers = 0;
                break;
            case 'm':
            case 'M':
                ch = KEY_ENTER;
                modifiers = 0;
                break;
            }
        }
        *key = modifiers | ch;
        return true;
    }

    for (size_t j = 0; j < ARRAY_COUNT(other_keys); j++) {
        if (ascii_streq_icase(str, other_keys[j].name)) {
            *key = modifiers | other_keys[j].key;
            return true;
        }
    }

    static_assert(ARRAY_COUNT(special_names) == NR_SPECIAL_KEYS);
    for (i = 0; i < NR_SPECIAL_KEYS; i++) {
        if (ascii_streq_icase(str, special_names[i])) {
            *key = modifiers | (KEY_SPECIAL_MIN + i);
            return true;
        }
    }

    return false;
}

const char *keycode_to_string(KeyCode k)
{
    static char buf[32];
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

    const KeyCode key = keycode_get_key(k);
    if (u_is_unicode(key)) {
        for (size_t j = 0; j < ARRAY_COUNT(other_keys); j++) {
            if (key == other_keys[j].key) {
                memcpy(buf + i, other_keys[j].name, sizeof(other_keys[0].name));
                return buf;
            }
        }
        u_set_char(buf, &i, key);
        buf[i] = '\0';
        return buf;
    }

    if (key >= KEY_SPECIAL_MIN && key <= KEY_SPECIAL_MAX) {
        const char *name = special_names[key - KEY_SPECIAL_MIN];
        memcpy(buf + i, name, sizeof(special_names[0]));
        return buf;
    }

    xsnprintf(buf, sizeof buf, "INVALID (0x%08X)", (unsigned int)k);
    return buf;
}
