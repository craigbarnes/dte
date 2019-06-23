#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "common.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"

size_t count_nl(const char *buf, size_t size)
{
    const char *end = buf + size;
    size_t nl = 0;

    while (buf < end) {
        buf = memchr(buf, '\n', end - buf);
        if (!buf) {
            break;
        }
        buf++;
        nl++;
    }
    return nl;
}

size_t count_strings(char **strings)
{
    size_t count = 0;
    while (strings[count]) {
        count++;
    }
    return count;
}

void free_strings(char **strings)
{
    for (size_t i = 0; strings[i]; i++) {
        free(strings[i]);
    }
    free(strings);
}
