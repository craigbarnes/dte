#include <errno.h>
#include <stdarg.h>
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

int xopen(const char *path, int flags, ...)
{
    va_list ap;
    va_start(ap, flags);
    unsigned int mode = 0;
    if (flags & O_CREAT) {
        // Note: "int" is used (instead of "mode_t") to prevent the following:
        // "error: second argument to 'va_arg' is of promotable type 'mode_t'
        // (aka 'unsigned short'); this va_arg has undefined behavior because
        // arguments will be promoted to 'int' [-Werror,-Wvarargs]"
        mode = va_arg(ap, unsigned int);
    }
    va_end(ap);

    int fd;
    do {
        fd = open(path, flags, (mode_t)mode);
    } while (fd < 0 && errno == EINTR);

    return fd;
}

int xclose(int fd)
{
    int r = close(fd);
    if (likely(r == 0 || errno != EINTR)) {
        return r;
    }

    // If the first close() call failed with EINTR, retry until
    // it succeeds or fails with a different error.
    do {
        r = close(fd);
    } while (r && errno == EINTR);

    // On some systems, when close() fails with EINTR, the descriptor
    // still gets closed anyway and calling close() again then fails
    // with EBADF. This is not really an "error" that can be handled
    // meaningfully, so we just return as if successful.
    //
    // Note that this is only safe to do in a single-threaded context,
    // where there's no risk of a concurrent open() call reusing the
    // same file descriptor.
    //
    // * http://www.daemonology.net/blog/2011-12-17-POSIX-close-is-broken.html
    // * https://sourceware.org/bugzilla/show_bug.cgi?id=14627
    if (r && errno == EBADF) {
        r = 0;
    }

    return r;
}
