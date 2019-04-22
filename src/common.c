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

// Returns size of file or -1 on error.
// For empty file *bufp is NULL otherwise *bufp is NUL-terminated.
ssize_t read_file(const char *filename, char **bufp)
{
    struct stat st;
    return stat_read_file(filename, bufp, &st);
}

ssize_t stat_read_file(const char *filename, char **bufp, struct stat *st)
{
    int fd = open(filename, O_RDONLY);
    *bufp = NULL;
    if (fd == -1) {
        return -1;
    }
    if (fstat(fd, st) == -1) {
        close(fd);
        return -1;
    }
    char *buf = xnew(char, st->st_size + 1);
    ssize_t r = xread(fd, buf, st->st_size);
    close(fd);
    if (r > 0) {
        buf[r] = '\0';
        *bufp = buf;
    } else {
        free(buf);
    }
    return r;
}

char *buf_next_line(char *buf, ssize_t *posp, ssize_t size)
{
    ssize_t pos = *posp;
    ssize_t avail = size - pos;
    char *line = buf + pos;
    char *nl = memchr(line, '\n', avail);
    if (nl) {
        *nl = 0;
        *posp += nl - line + 1;
    } else {
        line[avail] = '\0';
        *posp += avail;
    }
    return line;
}
