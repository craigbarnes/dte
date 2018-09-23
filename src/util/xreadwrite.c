#include <errno.h>
#include <unistd.h>
#include "xreadwrite.h"

ssize_t xread(int fd, void *buf, size_t count)
{
    char *b = buf;
    size_t pos = 0;
    do {
        ssize_t rc = read(fd, b + pos, count - pos);
        if (rc == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (rc == 0) {
            // eof
            break;
        }
        pos += rc;
    } while (count - pos > 0);
    return pos;
}

ssize_t xwrite(int fd, const void *buf, size_t count)
{
    const char *b = buf;
    const size_t count_save = count;
    do {
        ssize_t rc = write(fd, b, count);
        if (rc == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        b += rc;
        count -= rc;
    } while (count > 0);
    return count_save;
}
