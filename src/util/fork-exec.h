#ifndef UTIL_FORK_EXEC_H
#define UTIL_FORK_EXEC_H

#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include "macros.h"

static inline bool fd_set_flag(int fd, int flag, int get_cmd, int set_cmd, bool state)
{
    int flags = fcntl(fd, get_cmd);
    if (unlikely(flags < 0)) {
        return false;
    }
    int new_flags = state ? (flags | flag) : (flags & ~flag);
    return new_flags == flags || fcntl(fd, set_cmd, new_flags) != -1;
}

static inline bool fd_set_cloexec(int fd, bool cloexec)
{
    return fd_set_flag(fd, FD_CLOEXEC, F_GETFD, F_SETFD, cloexec);
}

static inline bool fd_set_nonblock(int fd, bool nonblock)
{
    return fd_set_flag(fd, O_NONBLOCK, F_GETFL, F_SETFL, nonblock);
}

int xpipe2(int fd[2], int flags) WARN_UNUSED_RESULT;
pid_t fork_exec(const char **argv, const char **env, int fd[3], bool drop_ctty) NONNULL_ARG(1) WARN_UNUSED_RESULT;
int wait_child(pid_t pid) WARN_UNUSED_RESULT;

#endif
