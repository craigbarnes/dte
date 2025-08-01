#ifndef UTIL_FD_H
#define UTIL_FD_H

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include "macros.h"

#define IS_STD_FD(fd) ((fd) >= STDIN_FILENO && (fd) <= STDERR_FILENO)

/*
 * "The arg values to F_GETFD, F_SETFD, F_GETFL, and F_SETFL all
 * represent flag values to allow for future growth. Applications
 * using these functions should do a read-modify-write operation on
 * them, rather than assuming that only the values defined by this
 * volume of POSIX.1-2017 are valid. It is a common error to forget
 * this, particularly in the case of F_SETFD."
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/fcntl.html#tag_16_122_07
 */
WARN_UNUSED_RESULT
static inline bool fd_set_flag(int fd, int flag, int get_cmd, int set_cmd, bool state)
{
    int flags = fcntl(fd, get_cmd);
    if (unlikely(flags < 0)) {
        return false;
    }
    int new_flags = state ? (flags | flag) : (flags & ~flag);
    return new_flags == flags || fcntl(fd, set_cmd, new_flags) != -1;
}

WARN_UNUSED_RESULT
static inline bool fd_set_cloexec(int fd, bool cloexec)
{
    return fd_set_flag(fd, FD_CLOEXEC, F_GETFD, F_SETFD, cloexec);
}

WARN_UNUSED_RESULT
static inline bool fd_set_nonblock(int fd, bool nonblock)
{
    return fd_set_flag(fd, O_NONBLOCK, F_GETFL, F_SETFL, nonblock);
}

WARN_UNUSED_RESULT
static inline bool fd_is_valid(int fd)
{
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

WARN_UNUSED_RESULT
static inline bool is_controlling_tty(int fd)
{
    /*
     * POSIX requires tcgetpgrp() to return -1 and set errno to ENOTTY if
     * `fd` isn't the controlling terminal, so this check alone should be
     * sufficient here. However, OpenBSD 7.5 and FreeBSD 14.0 don't seem
     * to conform to this requirement, so as a best-effort workaround we
     * also check isatty(). This means that the function name used here
     * isn't entirely accurate on those platforms, although it'd take a
     * very unusual set of stdio(3) redirections for it to be a problem
     * in practice (for the use cases in this codebase).
     *
     * See also: https://gitlab.com/craigbarnes/dte/-/issues/216#note_1939534295
     */
    return isatty(fd) && tcgetpgrp(fd) != -1;
}

int xpipe2(int fd[2], int flags) WARN_UNUSED_RESULT;
int xdup3(int oldfd, int newfd, int flags) WARN_UNUSED_RESULT;
int xfchown(int fd, uid_t owner, gid_t group) WARN_UNUSED_RESULT;
int xfchmod(int fd, mode_t mode) WARN_UNUSED_RESULT;
int xftruncate(int fd, off_t length) WARN_UNUSED_RESULT;
int xfsync(int fd) WARN_UNUSED_RESULT;

#endif
