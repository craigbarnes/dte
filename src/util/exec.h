#ifndef UTIL_EXEC_H
#define UTIL_EXEC_H

#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include "macros.h"

static inline bool fd_set_cloexec(int fd, bool cloexec)
{
    int flags = fcntl(fd, F_GETFD);
    if (unlikely(flags < 0)) {
        return false;
    }
    int new_flags = cloexec ? (flags | FD_CLOEXEC) : (flags & ~FD_CLOEXEC);
    return new_flags == flags || fcntl(fd, F_SETFD, new_flags) != -1;
}

int xpipe2(int fd[2], int flags) WARN_UNUSED_RESULT;
pid_t fork_exec(const char **argv, const char **env, int fd[3], bool drop_ctty) NONNULL_ARG(1) WARN_UNUSED_RESULT;
int wait_child(pid_t pid) WARN_UNUSED_RESULT;

#endif
