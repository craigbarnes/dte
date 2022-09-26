#ifndef UTIL_FORK_EXEC_H
#define UTIL_FORK_EXEC_H

#include <stdbool.h>
#include <sys/types.h>
#include "macros.h"

pid_t fork_exec(const char **argv, const char **env, int fd[3], bool drop_ctty) NONNULL_ARG(1) WARN_UNUSED_RESULT;
int wait_child(pid_t pid) WARN_UNUSED_RESULT;

#endif
