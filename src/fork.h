#ifndef FORK_H
#define FORK_H

void close_on_exec(int fd);
int pipe_close_on_exec(int fd[2]);
int fork_exec(char **argv, int fd[3]);
int wait_child(int pid);

#endif
