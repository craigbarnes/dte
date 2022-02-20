#include "linux.h"
#include "xterm.h"

ssize_t linux_parse_key(const char *buf, size_t length, KeyCode *k)
{
    if (length < 3 || buf[0] != '\033' || buf[1] != '[' || buf[2] != '[') {
        return xterm_parse_key(buf, length, k);
    }

    if (unlikely(length == 3)) {
        return -1;
    }

    // Letters A-E represent keys F1-F5
    char c = buf[3];
    if (c >= 'A' && c <= 'E') {
        *k = KEY_F1 + (c - 'A');
        return 4;
    }

    return 0;
}
