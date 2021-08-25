#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "readfile.h"
#include "xreadwrite.h"

ssize_t read_file(const char *filename, char **bufp)
{
    int fd = xopen(filename, O_RDONLY | O_CLOEXEC, 0);
    if (fd == -1) {
        goto error_noclose;
    }

    int saved_errno;
    struct stat st;
    if (unlikely(fstat(fd, &st) == -1)) {
        goto error;
    }
    if (unlikely(S_ISDIR(st.st_mode))) {
        errno = EISDIR;
        goto error;
    }

    char *buf = malloc(st.st_size + 1);
    if (unlikely(!buf)) {
        goto error;
    }

    ssize_t r = xread(fd, buf, st.st_size);
    if (unlikely(r < 0)) {
        free(buf);
        goto error;
    }

    buf[r] = '\0';
    *bufp = buf;
    xclose(fd);
    return r;

error:
    saved_errno = errno;
    xclose(fd);
    errno = saved_errno;
error_noclose:
    *bufp = NULL;
    return -1;
}
