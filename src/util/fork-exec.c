#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "fork-exec.h"
#include "debug.h"
#include "fd.h"
#include "log.h"
#include "terminal/ioctl.h"
#include "xreadwrite.h"

// Reset ignored signal dispositions (i.e. as originally set up by
// set_basic_signal_dispositions()) to SIG_DFL
static bool reset_ignored_signals(void)
{
    // Note that handled signals don't need to be restored here, since
    // they're necessarily and automatically reset after exec(3p)
    static const int ignored_signals[] = {
        SIGINT, SIGQUIT, SIGTSTP,
        SIGTTIN, SIGTTOU, SIGXFSZ,
        SIGPIPE, SIGUSR1, SIGUSR2,
    };

    struct sigaction action = {.sa_handler = SIG_DFL};
    if (unlikely(sigemptyset(&action.sa_mask) != 0)) {
        return false;
    }

    for (size_t i = 0; i < ARRAYLEN(ignored_signals); i++) {
        int r = sigaction(ignored_signals[i], &action, NULL);
        if (unlikely(r != 0)) {
            return false;
        }
    }

    return true;
}

static noreturn void handle_child(const char **argv, const char **env, int fd[3], int error_fd, bool drop_ctty)
{
    int error;
    int nr_fds = 3;
    bool move = error_fd < nr_fds;
    int max = error_fd;

    if (drop_ctty) {
        term_drop_controlling_tty(STDIN_FILENO);
    }

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
            if (!fd_set_cloexec(fd[i], false)) {
                goto error;
            }
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
            int r = value ? setenv(name, value, 1) : unsetenv(name);
            if (unlikely(r != 0)) {
                goto error;
            }
        }
    }

    if (!reset_ignored_signals()) {
        goto error;
    }

    execvp(argv[0], (char**)argv);

error:
    error = errno;
    error = xwrite(error_fd, &error, sizeof(error));
    exit(42);
}

static pid_t xwaitpid(pid_t pid, int *status, int options)
{
    pid_t ret;
    do {
        ret = waitpid(pid, status, options);
    } while (unlikely(ret < 0 && errno == EINTR));
    return ret;
}

pid_t fork_exec(const char **argv, const char **env, int fd[3], bool drop_ctty)
{
    int ep[2];
    if (xpipe2(ep, O_CLOEXEC) != 0) {
        return -1;
    }

    const pid_t pid = fork();
    if (unlikely(pid == -1)) {
        int saved_errno = errno;
        xclose(ep[0]);
        xclose(ep[1]);
        errno = saved_errno;
        return -1;
    }

    if (pid == 0) {
        // Child
        handle_child(argv, env, fd, ep[1], drop_ctty);
        BUG("handle_child() should never return");
        return -1;
    }

    // Parent
    xclose(ep[1]);
    int error = 0;
    ssize_t rc = xread(ep[0], &error, sizeof(error));
    int xread_errno = errno;
    xclose(ep[0]);
    BUG_ON(rc > sizeof(error));

    if (rc == 0) {
        // Child exec was successful
        return pid;
    }

    if (unlikely(rc != sizeof(error))) {
        error = (rc < 0) ? xread_errno : EPIPE;
    }

    int status;
    xwaitpid(pid, &status, 0);
    errno = error;
    return -1;
}

int wait_child(pid_t pid)
{
    int status;
    if (unlikely(xwaitpid(pid, &status, 0) < 0)) {
        return -errno;
    }

    if (likely(WIFEXITED(status))) {
        return WEXITSTATUS(status) & 0xFF;
    }

    if (likely(WIFSIGNALED(status))) {
        return WTERMSIG(status) << 8;
    }

    LOG_ERROR("unhandled waitpid() status: %d", status);
    return -EINVAL;
}
