#include "rxvt.h"
#include "xterm.h"

/*
 rxvt uses some key codes that differ from the ones used by xterm.
 They don't conflict or overlap with the xterm codes, but specifically
 aren't handled by xterm_parse_key() because some of them violate the
 ECMA-48 spec (notably, the ones ending with '$', which isn't a valid
 final byte).
*/
ssize_t rxvt_parse_key(const char *buf, size_t length, KeyCode *k)
{
    KeyCode extra_mods = 0;
    size_t extra_bytes = 0;
    if (length < 2 || buf[0] != '\033') {
        goto xterm;
    }

    KeyCode mods;
    switch (buf[1]) {
    case 'O':
        mods = MOD_CTRL;
        goto final;
    case '[':
        mods = MOD_SHIFT;
        final:
        if (length < 3) {
            return -1;
        }
        switch (buf[2]) {
        case 'a': // Up
        case 'b': // Down
        case 'c': // Right
        case 'd': // Left
            *k = mods | (KEY_UP + (buf[2] - 'a'));
            return 3;
        }
        break;
    case '\033':
        extra_mods = MOD_META;
        extra_bytes = 1;
        buf++;
        length--;
        break;
    }

    if (length < 4 || buf[1] != '[') {
        goto xterm;
    }

    KeyCode key;
    ssize_t n;
    switch (buf[3]) {
    case '~': key = 0; break;
    case '^': key = MOD_CTRL; break;
    case '$': key = MOD_SHIFT; break;
    case '@': key = MOD_SHIFT | MOD_CTRL; break;
    default: goto xterm;
    }

    switch (buf[2]) {
    case '2': key |= KEY_INSERT; break;
    case '3': key |= KEY_DELETE; break;
    case '5': key |= KEY_PAGE_UP; break;
    case '6': key |= KEY_PAGE_DOWN; break;
    case '7': key |= KEY_HOME; break;
    case '8': key |= KEY_END; break;
    default: goto xterm;
    }

    *k = key | extra_mods;
    return 4 + extra_bytes;

xterm:
    n = xterm_parse_key(buf, length, &key);
    if (n <= 0) {
        return n;
    }
    if (unlikely(key == KEY_IGNORE && extra_mods)) {
        extra_mods = 0;
    }
    *k = key | extra_mods;
    return n + extra_bytes;
}
