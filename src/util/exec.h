#ifndef UTIL_EXEC_H
#define UTIL_EXEC_H

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include "macros.h"

static inline void close_on_exec(int fd)
{
    fcntl(fd, F_SETFD, FD_CLOEXEC);
}

static inline int xpipe2(int fd[2], int flags)
{
    int ret = pipe(fd);
    if (ret == 0) {
        fcntl(fd[0], F_SETFD, flags);
        fcntl(fd[1], F_SETFD, flags);
    }
    return ret;
}

pid_t fork_exec(char **argv, int fd[3]) NONNULL_ARGS;
int wait_child(pid_t pid);

#endif
