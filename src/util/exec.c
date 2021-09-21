#include "../../build/feature.h" // Must be first include
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "exec.h"
#include "debug.h"
#include "xreadwrite.h"

bool pipe_cloexec(int fd[2])
{
#ifdef HAVE_PIPE2
    if (likely(pipe2(fd, O_CLOEXEC) == 0)) {
        return true;
    }
    // If pipe2() fails with ENOSYS, it means the function is just a stub
    // and not actually supported. In that case, the pure POSIX fallback
    // implementation should be tried instead. In other cases, the failure
    // is probably caused by a normal error condition.
    if (errno != ENOSYS) {
        return false;
    }
#endif

    if (pipe(fd) != 0) {
        return false;
    }
    fd_set_cloexec(fd[0], true);
    fd_set_cloexec(fd[1], true);
    return true;
}

static int xdup3(int oldfd, int newfd, int flags)
{
    int fd;
#ifdef HAVE_DUP3
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
        fd_set_cloexec(fd, true);
    }
    return fd;
}

static noreturn void handle_child(char **argv, const char **env, int fd[3], int error_fd)
{
    int error;
    int nr_fds = 3;
    bool move = error_fd < nr_fds;
    int max = error_fd;

    // Find if we must move fds out of the way
    for (int i = 0; i < nr_fds; i++) {
        if (fd[i] > max) {
            max = fd[i];
        }
        if (fd[i] < i) {
            move = true;
        }
    }

    if (move) {
        int next_free = max + 1;
        if (error_fd < nr_fds) {
            error_fd = xdup3(error_fd, next_free++, O_CLOEXEC);
            if (error_fd < 0) {
                goto error;
            }
        }
        for (int i = 0; i < nr_fds; i++) {
            if (fd[i] < i) {
                fd[i] = xdup3(fd[i], next_free++, O_CLOEXEC);
                if (fd[i] < 0) {
                    goto error;
                }
            }
        }
    }

    // Now it is safe to duplicate fds in this order
    for (int i = 0; i < nr_fds; i++) {
        if (i == fd[i]) {
            // Clear FD_CLOEXEC flag
            fd_set_cloexec(fd[i], false);
        } else {
            if (xdup3(fd[i], i, 0) < 0) {
                goto error;
            }
        }
    }

    if (env) {
        for (size_t i = 0; env[i]; i += 2) {
            const char *name = env[i];
            const char *value = env[i + 1];
            int r = value ? setenv(name, value, true) : unsetenv(name);
            if (unlikely(r != 0)) {
                goto error;
            }
        }
    }

    // Reset ignored signals to SIG_DFL (see exec(3p))
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    sigemptyset(&act.sa_mask);
    act.sa_handler = SIG_DFL;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
    sigaction(SIGTSTP, &act, NULL);
    sigaction(SIGPIPE, &act, NULL);
    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act, NULL);

    execvp(argv[0], argv);

error:
    error = errno;
    error = write(error_fd, &error, sizeof(error));
    exit(42);
}

pid_t fork_exec(char **argv, const char **env, int fd[3])
{
    int ep[2];
    if (!pipe_cloexec(ep)) {
        return -1;
    }

    const pid_t pid = fork();
    if (unlikely(pid < 0)) {
        const int saved_errno = errno;
        xclose(ep[0]);
        xclose(ep[1]);
        errno = saved_errno;
        return pid;
    }

    if (pid == 0) {
        // Child
        handle_child(argv, env, fd, ep[1]);
        BUG("handle_child() should never return");
    }

    // Parent
    xclose(ep[1]);
    int error = 0;
    const ssize_t rc = read(ep[0], &error, sizeof(error));
    const int saved_errno = errno;
    xclose(ep[0]);

    if (rc > 0 && rc != sizeof(error)) {
        error = EPIPE;
    }
    if (rc < 0) {
        error = saved_errno;
    }
    if (rc == 0) {
        // Child exec was successful
        return pid;
    }

    int status;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
        ;
    }
    errno = error;
    return -1;
}

int wait_child(pid_t pid)
{
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) {
            continue;
        }
        return -errno;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status) & 0xFF;
    }

    if (WIFSIGNALED(status)) {
        return WTERMSIG(status) << 8;
    }

    if (WIFSTOPPED(status)) {
        return WSTOPSIG(status) << 8;
    }

    return -EINVAL;
}
