#include "linux.h"
#include "parse.h"
#include "util/str-util.h"

ssize_t linux_parse_key(const char *buf, size_t length, KeyCode *k)
{
    if (length < 3 || !mem_equal(buf, "\033[[", 3)) {
        return term_parse_sequence(buf, length, k);
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
