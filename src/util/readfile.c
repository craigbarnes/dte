#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "readfile.h"
#include "xreadwrite.h"

// Read the contents of a file into memory, if smaller than `size_limit`
ssize_t read_file(const char *filename, char **bufp, size_t size_limit)
{
    int fd = xopen(filename, O_RDONLY | O_CLOEXEC, 0);
    if (fd == -1) {
        goto error_noclose;
    }

    struct stat st;
    if (unlikely(fstat(fd, &st) == -1)) {
        goto error;
    }
    if (unlikely(S_ISDIR(st.st_mode))) {
        errno = EISDIR;
        goto error;
    }

    off_t size = st.st_size;
    size_limit = size_limit ? MIN(size_limit, SSIZE_MAX) : SSIZE_MAX;
    if (unlikely(size > size_limit)) {
        errno = EFBIG;
        goto error;
    }

    char *buf = malloc(size + 1);
    if (unlikely(!buf)) {
        goto error;
    }

    ssize_t r = xread_all(fd, buf, size);
    if (unlikely(r < 0)) {
        free(buf);
        goto error;
    }

    buf[r] = '\0';
    *bufp = buf;
    xclose(fd);
    return r;

error:
    xclose(fd);
error_noclose:
    *bufp = NULL;
    return -1;
}
