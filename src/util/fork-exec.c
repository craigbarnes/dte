#include "feature.h"
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "fork-exec.h"
#include "debug.h"
#include "fd.h"
#include "log.h"
#include "numtostr.h"
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

// NOLINTNEXTLINE(readability-function-size)
static noreturn void child_process_exec (
    const char **argv,
    const int fd[3],
    int error_fd, // Pipe to parent, for communicating pre-exec errors
    unsigned int lines,
    unsigned int columns,
    bool drop_ctty
) {
    if (drop_ctty) {
        term_drop_controlling_tty(STDIN_FILENO);
    }

    for (int i = STDIN_FILENO; i <= STDERR_FILENO; i++) {
        int f = fd[i];
        bool ok = (i == f) ? fd_set_cloexec(f, false) : xdup3(f, i, 0) >= 0;
        if (unlikely(!ok)) {
            goto error;
        }
    }

    if (lines) {
        setenv("LINES", uint_to_str(lines), 1);
    }
    if (columns) {
        setenv("COLUMNS", uint_to_str(columns), 1);
    }

    if (unlikely(!reset_ignored_signals())) {
        goto error;
    }

    execvp(argv[0], (char**)argv);

error:;
    int error = errno;
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

pid_t fork_exec (
    const char **argv,
    int fd[3],
    unsigned int lines,
    unsigned int columns,
    bool drop_ctty
) {
    // Create an "error pipe" before forking, so that child_process_exec()
    // can signal pre-exec errors and allow the parent differentiate them
    // from a successful exec(3) with a non-zero exit status
    int ep[2];
    if (xpipe2(ep, O_CLOEXEC) != 0) {
        return -1;
    }

    BUG_ON(ep[0] <= STDERR_FILENO);
    BUG_ON(ep[1] <= STDERR_FILENO);
    BUG_ON(fd[0] <= STDERR_FILENO && fd[0] != 0);
    BUG_ON(fd[1] <= STDERR_FILENO && fd[1] != 1);
    BUG_ON(fd[2] <= STDERR_FILENO && fd[2] != 2);

    const pid_t pid = fork();
    if (unlikely(pid == -1)) {
        xclose(ep[0]);
        xclose(ep[1]);
        return -1;
    }

    if (pid == 0) {
        // Child
        child_process_exec(argv, fd, ep[1], lines, columns, drop_ctty);
        BUG("child_process_exec() should never return");
        return -1;
    }

    // Parent
    xclose(ep[1]);
    int error = 0;
    ssize_t rc = xread(ep[0], &error, sizeof(error));
    xclose(ep[0]);
    BUG_ON(rc > sizeof(error));

    if (rc == 0) {
        // Child exec was successful
        return pid;
    }

    if (unlikely(rc != sizeof(error))) {
        error = (rc < 0) ? errno : EPIPE;
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
