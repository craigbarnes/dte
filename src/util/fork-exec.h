#ifndef UTIL_FORK_EXEC_H
#define UTIL_FORK_EXEC_H

#include <stdbool.h>
#include <sys/types.h>
#include "macros.h"

WARN_UNUSED_RESULT NONNULL_ARGS
pid_t fork_exec (
    const char **argv,
    int fd[3],
    int progfd,
    unsigned int lines,
    unsigned int columns,
    bool drop_ctty
);

int wait_child(pid_t pid) WARN_UNUSED_RESULT;

#endif
