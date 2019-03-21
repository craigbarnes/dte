#ifndef UTIL_EXEC_H
#define UTIL_EXEC_H

#include <sys/types.h>
#include "macros.h"

void close_on_exec(int fd);
int pipe_close_on_exec(int fd[2]);
pid_t fork_exec(char **argv, int fd[3]) NONNULL_ARGS;
int wait_child(pid_t pid);

#endif
