#include "linux.h"
#include "xterm.h"

ssize_t linux_parse_key(const char *buf, size_t length, KeyCode *k)
{
    if (length < 3 || buf[0] != '\033' || buf[1] != '[') {
        goto xterm;
    }

    size_t i = 2;
    if (buf[i++] == '[') {
        if (unlikely(i >= length)) {
            return -1;
        }
        char ch = buf[i++];
        switch (ch) {
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

xterm:
    return xterm_parse_key(buf, length, k);
}
