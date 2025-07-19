#include <limits.h>
#include <unistd.h>
#include "xreadwrite.h"

#if defined(__APPLE__)
    // See: https://gitlab.com/craigbarnes/dte/-/issues/201
    #define MAX_RW_COUNT ((size_t)INT_MAX)
#elif defined(__linux__) && 0x7ffff000 <= SSIZE_MAX
    // Unlike macOS, trying to read or write more than 0x7ffff000 bytes
    // on Linux simply causes the syscall to return (at most) the upper
    // limit, rather than indicating an error condition. Thus, using
    // this value here is done simply because it's a more appropriate
    // value than SSIZE_MAX, not because it's actually needed for
    // correct functioning of xread_all().
    // See also:
    // • https://man7.org/linux/man-pages/man2/read.2.html#NOTES
    // • https://man7.org/linux/man-pages/man2/write.2.html#NOTES
    // • https://stackoverflow.com/a/70370002
    #define MAX_RW_COUNT ((size_t)0x7ffff000)
#else
    #define MAX_RW_COUNT ((size_t)SSIZE_MAX)
#endif

static size_t rwsize(size_t count)
{
    return MIN(count, MAX_RW_COUNT);
}

// NOLINTBEGIN(*-unsafe-functions)

ssize_t xread(int fd, void *buf, size_t count)
{
    ssize_t r;
    do {
        r = read(fd, buf, count);
    } while (unlikely(r < 0 && errno == EINTR));
    return r;
}

ssize_t xwrite(int fd, const void *buf, size_t count)
{
    ssize_t r;
    do {
        r = write(fd, buf, count);
    } while (unlikely(r < 0 && errno == EINTR));
    return r;
}

ssize_t xread_all(int fd, void *buf, size_t count)
{
    char *b = buf;
    size_t pos = 0;
    do {
        ssize_t rc = read(fd, b + pos, rwsize(count - pos));
        if (unlikely(rc < 0)) {
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
    } while (pos < count);
    return pos;
}

ssize_t xwrite_all(int fd, const void *buf, size_t count)
{
    const char *b = buf;
    const size_t count_save = count;
    do {
        ssize_t rc = write(fd, b, rwsize(count));
        if (unlikely(rc < 0)) {
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

// Like close(3), but with the following differences:
// • Retries the operation, if interrupted by a caught signal (EINTR)
// • Handles EINPROGRESS as if successful (i.e. by returning 0)
// • Always restores errno(3) to the previous value before returning
// • Returns an <errno.h> value, if a meaningful error occurred, or otherwise 0
SystemErrno xclose(int fd)
{
    const SystemErrno saved_errno = errno;
    int r = close(fd);
    if (likely(r == 0 || errno != EINTR)) {
        errno = saved_errno;
        // Treat EINPROGRESS the same as r == 0
        // (https://git.musl-libc.org/cgit/musl/commit/?id=82dc1e2e783815e00a90cd)
        return (r && errno != EINPROGRESS) ? errno : 0;
    }

    // If the first close() call failed with EINTR, retry until it succeeds
    // or fails with a different error
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
    // • http://www.daemonology.net/blog/2011-12-17-POSIX-close-is-broken.html
    // • https://ewontfix.com/4/
    // • https://sourceware.org/bugzilla/show_bug.cgi?id=14627
    // • https://austingroupbugs.net/view.php?id=529#c1200
    errno = saved_errno;
    return (r && errno != EBADF) ? errno : 0;
}

// NOLINTEND(*-unsafe-functions)
