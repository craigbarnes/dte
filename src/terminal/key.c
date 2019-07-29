#include <string.h>
#include "key.h"
#include "../debug.h"
#include "../util/ascii.h"
#include "../util/utf8.h"

// Note: these strings must be kept in sync with the enum in key.h
static const char special_names[][8] = {
    "insert",
    "delete",
    "up",
    "down",
    "right",
    "left",
    "pgdown",
    "end",
    "pgup",
    "home",
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
};
static_assert(ARRAY_COUNT(special_names) == NR_SPECIAL_KEYS);

static size_t parse_modifiers(const char *const str, KeyCode *modifiersp)
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

bool parse_key(KeyCode *key, const char *str)
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
                ch = '\t';
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
    if (ascii_streq_icase(str, "space")) {
        *key = modifiers | ' ';
        return true;
    }
    if (ascii_streq_icase(str, "tab")) {
        *key = modifiers | '\t';
        return true;
    }
    if (ascii_streq_icase(str, "enter")) {
        *key = modifiers | KEY_ENTER;
        return true;
    }

    for (i = 0; i < NR_SPECIAL_KEYS; i++) {
        if (ascii_streq_icase(str, special_names[i])) {
            *key = modifiers | (KEY_SPECIAL_MIN + i);
            return true;
        }
    }

    return false;
}

#define COPY(dest, src) memcpy(dest, src, STRLEN(src) + 1)

const char *key_to_string(KeyCode k)
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

    char *const ptr = buf + i;
    const KeyCode key = keycode_get_key(k);
    if (u_is_unicode(key)) {
        switch (key) {
        case '\t':
            COPY(ptr, "tab");
            break;
        case KEY_ENTER:
            COPY(ptr, "enter");
            break;
        case ' ':
            COPY(ptr, "space");
            break;
        default:
            u_set_char(buf, &i, key);
            buf[i] = '\0';
        }
    } else if (key >= KEY_SPECIAL_MIN && key <= KEY_SPECIAL_MAX) {
        static_assert(sizeof(special_names[0]) == 8);
        memcpy (
            ptr,
            special_names[key - KEY_SPECIAL_MIN],
            sizeof(special_names[0])
        );
    } else if (key == KEY_PASTE) {
        COPY(ptr, "paste");
    } else {
        COPY(ptr, "???");
    }

    return buf;
}
