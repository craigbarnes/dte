#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "readfile.h"
#include "xmalloc.h"
#include "xreadwrite.h"

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
    char *buf = xmalloc(st->st_size + 1);
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
        *nl = '\0';
        *posp += nl - line + 1;
    } else {
        line[avail] = '\0';
        *posp += avail;
    }
    return line;
}

StringView buf_slice_next_line(const char *buf, ssize_t *posp, ssize_t size)
{
    ssize_t pos = *posp;
    ssize_t avail = size - pos;
    const char *line = buf + pos;
    const char *nl = memchr(line, '\n', avail);
    size_t line_length = nl ? (nl - line + 1) : avail;
    *posp += line_length;
    return string_view(line, line_length);
}
