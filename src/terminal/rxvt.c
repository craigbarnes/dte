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
    if (length >= 4 && buf[0] == '\033' && buf[1] == '[') {
        KeyCode key;
        switch (buf[3]) {
        case '~': key = 0; break;
        case '^': key = MOD_CTRL; break;
        case '$': key = MOD_SHIFT; break;
        case '@': key = MOD_SHIFT | MOD_CTRL; break;
        default: goto xterm;
        }
        switch (buf[2]) {
        case '3': key |= KEY_DELETE; break;
        case '5': key |= KEY_PAGE_UP; break;
        case '6': key |= KEY_PAGE_DOWN; break;
        case '7': key |= KEY_HOME; break;
        case '8': key |= KEY_END; break;
        default: goto xterm;
        }
        *k = key;
        return 4;
    }

xterm:
    return xterm_parse_key(buf, length, k);
}
