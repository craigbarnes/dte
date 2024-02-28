#include "compat.h"
#include <errno.h>
#include <sys/stat.h>
#include "fd.h"
#include "debug.h"
#include "xreadwrite.h"

int xpipe2(int fd[2], int flags)
{
    BUG_ON((flags & (O_CLOEXEC | O_NONBLOCK)) != flags);

#if HAVE_PIPE2
    if (likely(pipe2(fd, flags) == 0)) {
        return 0;
    }
    // If pipe2() fails with ENOSYS, it means the function is just a stub
    // and not actually supported. In that case, the pure POSIX fallback
    // implementation should be tried instead. In other cases, the failure
    // is probably caused by a normal error condition.
    if (errno != ENOSYS) {
        return -1;
    }
#endif

    // NOLINTNEXTLINE(android-cloexec-pipe)
    if (unlikely(pipe(fd) != 0)) {
        return -1;
    }

    if (flags & O_CLOEXEC) {
        if (unlikely(!fd_set_cloexec(fd[0], true) || !fd_set_cloexec(fd[1], true))) {
            goto error;
        }
    }
    if (flags & O_NONBLOCK) {
        if (unlikely(!fd_set_nonblock(fd[0], true) || !fd_set_nonblock(fd[1], true))) {
            goto error;
        }
    }

    return 0;

error:;
    int e = errno;
    xclose(fd[0]);
    xclose(fd[1]);
    errno = e;
    return -1;
}

int xdup3(int oldfd, int newfd, int flags)
{
    BUG_ON((flags & O_CLOEXEC) != flags);
    int fd;

#if HAVE_DUP3
    do {
        fd = dup3(oldfd, newfd, flags);
    } while (unlikely(fd < 0 && errno == EINTR));

    if (fd != -1 || errno != ENOSYS) {
        return fd;
    }

    // If execution reaches this point, dup3() failed with ENOSYS
    // ("function not supported"), so we fall through to the pure
    // POSIX implementation below.
#endif

    if (unlikely(oldfd == newfd)) {
        // Replicate dup3() behaviour:
        errno = EINVAL;
        return -1;
    }

    do {
        fd = dup2(oldfd, newfd);
    } while (unlikely(fd < 0 && errno == EINTR));

    if (fd >= 0 && (flags & O_CLOEXEC)) {
        (void)!fd_set_cloexec(fd, true);
    }

    return fd;
}

int xfchown(int fd, uid_t owner, gid_t group)
{
    int r;
    do {
        r = fchown(fd, owner, group);
    } while (unlikely(r != 0 && errno == EINTR));
    return r;
}

int xfchmod(int fd, mode_t mode)
{
    int r;
    do {
        r = fchmod(fd, mode);
    } while (unlikely(r != 0 && errno == EINTR));
    return r;
}

int xftruncate(int fd, off_t length)
{
    int r;
    do {
        r = ftruncate(fd, length);
    } while (unlikely(r != 0 && errno == EINTR));
    return r;
}
