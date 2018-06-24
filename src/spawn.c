#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "spawn.h"
#include "editor.h"
#include "buffer.h"
#include "regexp.h"
#include "error.h"
#include "util/str.h"
#include "msg.h"
#include "term.h"

static void close_on_exec(int fd)
{
    fcntl(fd, F_SETFD, FD_CLOEXEC);
}

static int pipe_close_on_exec(int fd[2])
{
    int ret = pipe(fd);
    if (ret == 0) {
        close_on_exec(fd[0]);
        close_on_exec(fd[1]);
    }
    return ret;
}

static int dup_close_on_exec(int oldfd, int newfd)
{
    if (dup2(oldfd, newfd) < 0) {
        return -1;
    }
    close_on_exec(newfd);
    return newfd;
}

static void handle_child(char **argv, int fd[3], int error_fd)
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
            error_fd = dup_close_on_exec(error_fd, next_free++);
            if (error_fd < 0) {
                goto out;
            }
        }
        for (int i = 0; i < nr_fds; i++) {
            if (fd[i] < i) {
                fd[i] = dup_close_on_exec(fd[i], next_free++);
                if (fd[i] < 0) {
                    goto out;
                }
            }
        }
    }

    // Now it is safe to duplicate fds in this order
    for (int i = 0; i < nr_fds; i++) {
        if (i == fd[i]) {
            // Clear FD_CLOEXEC flag
            fcntl(fd[i], F_SETFD, 0);
        } else {
            if (dup2(fd[i], i) < 0) {
                goto out;
            }
        }
    }

    // Unignore signals (see man page exec(3p) for more information)
    set_signal_handler(SIGINT, SIG_DFL);
    set_signal_handler(SIGQUIT, SIG_DFL);

    execvp(argv[0], argv);
out:
    error = errno;
    error = write(error_fd, &error, sizeof(error));
    exit(42);
}

static int fork_exec(char **argv, int fd[3])
{
    int error = 0;
    int ep[2];

    if (pipe_close_on_exec(ep)) {
        return -1;
    }

    const pid_t pid = fork();
    if (pid < 0) {
        error = errno;
        close(ep[0]);
        close(ep[1]);
        errno = error;
        return pid;
    }
    if (!pid) {
        handle_child(argv, fd, ep[1]);
    }

    close(ep[1]);
    const ssize_t rc = read(ep[0], &error, sizeof(error));
    if (rc > 0 && rc != sizeof(error)) {
        error = EPIPE;
    }
    if (rc < 0) {
        error = errno;
    }
    close(ep[0]);

    if (!rc) {
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

static int wait_child(int pid)
{
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) {
            continue;
        }
        return -errno;
    }
    if (WIFEXITED(status)) {
        return (unsigned char)WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return WTERMSIG(status) << 8;
    }
    if (WIFSTOPPED(status)) {
        return WSTOPSIG(status) << 8;
    }
#if defined(WIFCONTINUED)
    if (WIFCONTINUED(status)) {
        return SIGCONT << 8;
    }
#endif
    return -EINVAL;
}

static void handle_error_msg(Compiler *c, char *str)
{
    size_t i, len;

    for (i = 0; str[i]; i++) {
        if (str[i] == '\n') {
            str[i] = 0;
            break;
        }
        if (str[i] == '\t') {
            str[i] = ' ';
        }
    }
    len = i;
    if (len == 0) {
        return;
    }

    for (i = 0; i < c->error_formats.count; i++) {
        const ErrorFormat *p = c->error_formats.ptrs[i];
        PointerArray m = PTR_ARRAY_INIT;

        if (!regexp_exec_sub(&p->re, str, len, &m, 0)) {
            continue;
        }
        if (!p->ignore) {
            Message *msg = new_message(m.ptrs[p->msg_idx]);
            msg->loc = xnew0(FileLocation, 1);
            msg->loc->filename = p->file_idx < 0 ? NULL : xstrdup(m.ptrs[p->file_idx]);
            msg->loc->line = p->line_idx < 0 ? 0 : atoi(m.ptrs[p->line_idx]);
            msg->loc->column = p->column_idx < 0 ? 0 : atoi(m.ptrs[p->column_idx]);
            add_message(msg);
        }
        ptr_array_free(&m);
        return;
    }
    add_message(new_message(str));
}

static void read_errors(Compiler *c, int fd, bool quiet)
{
    FILE *f = fdopen(fd, "r");
    char line[4096];

    if (!f) {
        // Should not happen
        return;
    }

    while (fgets(line, sizeof(line), f)) {
        if (!quiet) {
            fputs(line, stderr);
        }
        handle_error_msg(c, line);
    }
    fclose(f);
}

static void filter(int rfd, int wfd, FilterData *fdata)
{
    size_t wlen = 0;
    String buf = STRING_INIT;

    if (!fdata->in_len) {
        close(wfd);
        wfd = -1;
    }
    while (1) {
        fd_set rfds, wfds;
        fd_set *wfdsp = NULL;
        int fd_high = rfd;

        FD_ZERO(&rfds);
        FD_SET(rfd, &rfds);

        if (wfd >= 0) {
            FD_ZERO(&wfds);
            FD_SET(wfd, &wfds);
            wfdsp = &wfds;
        }
        if (wfd > fd_high) {
            fd_high = wfd;
        }

        if (select(fd_high + 1, &rfds, wfdsp, NULL, NULL) < 0) {
            if (errno == EINTR) {
                continue;
            }
            error_msg("select: %s", strerror(errno));
            break;
        }

        if (FD_ISSET(rfd, &rfds)) {
            char data[8192];

            ssize_t rc = read(rfd, data, sizeof(data));
            if (rc < 0) {
                error_msg("read: %s", strerror(errno));
                break;
            }
            if (!rc) {
                if (wlen < fdata->in_len) {
                    error_msg("Command did not read all data.");
                }
                break;
            }
            string_add_buf(&buf, data, (size_t) rc);
        }
        if (wfdsp && FD_ISSET(wfd, &wfds)) {
            ssize_t rc = write(wfd, fdata->in + wlen, fdata->in_len - wlen);
            if (rc < 0) {
                error_msg("write: %s", strerror(errno));
                break;
            }
            wlen += (size_t) rc;
            if (wlen == fdata->in_len) {
                if (close(wfd)) {
                    error_msg("close: %s", strerror(errno));
                    break;
                }
                wfd = -1;
            }
        }
    }
    fdata->out = string_steal(&buf, &fdata->out_len);
}

static int open_dev_null(int flags)
{
    int fd = open("/dev/null", flags);
    if (fd < 0) {
        error_msg("Error opening /dev/null: %s", strerror(errno));
    } else {
        close_on_exec(fd);
    }
    return fd;
}

static int handle_child_error(int pid)
{
    int ret = wait_child(pid);

    if (ret < 0) {
        error_msg("waitpid: %s", strerror(errno));
    } else if (ret >= 256) {
        error_msg("Child received signal %d", ret >> 8);
    } else if (ret) {
        error_msg("Child returned %d", ret);
    }
    return ret;
}

int spawn_filter(char **argv, FilterData *data)
{
    int p0[2] = {-1, -1};
    int p1[2] = {-1, -1};
    int dev_null = -1;
    int fd[3], pid;

    data->out = NULL;
    data->out_len = 0;

    if (pipe_close_on_exec(p0) || pipe_close_on_exec(p1)) {
        error_msg("pipe: %s", strerror(errno));
        goto error;
    }
    dev_null = open_dev_null(O_WRONLY);
    if (dev_null < 0) {
        goto error;
    }

    fd[0] = p0[0];
    fd[1] = p1[1];
    fd[2] = dev_null;
    pid = fork_exec(argv, fd);
    if (pid < 0) {
        error_msg("Error: %s", strerror(errno));
        goto error;
    }

    close(dev_null);
    close(p0[0]);
    close(p1[1]);
    filter(p1[0], p0[1], data);
    close(p1[0]);
    close(p0[1]);

    if (handle_child_error(pid)) {
        return -1;
    }
    return 0;
error:
    close(p0[0]);
    close(p0[1]);
    close(p1[0]);
    close(p1[1]);
    close(dev_null);
    return -1;
}

void spawn_compiler(char **args, SpawnFlags flags, Compiler *c)
{
    bool read_stdout = !!(flags & SPAWN_READ_STDOUT);
    bool prompt = !!(flags & SPAWN_PROMPT);
    bool quiet = !!(flags & SPAWN_QUIET);
    int pid, dev_null, p[2], fd[3];

    fd[0] = open_dev_null(O_RDONLY);
    if (fd[0] < 0) {
        return;
    }
    dev_null = open_dev_null(O_WRONLY);
    if (dev_null < 0) {
        close(fd[0]);
        return;
    }
    if (pipe_close_on_exec(p)) {
        error_msg("pipe: %s", strerror(errno));
        close(dev_null);
        close(fd[0]);
        return;
    }

    if (read_stdout) {
        fd[1] = p[1];
        fd[2] = quiet ? dev_null : 2;
    } else {
        fd[1] = quiet ? dev_null : 1;
        fd[2] = p[1];
    }

    if (!quiet) {
        editor.child_controls_terminal = true;
        ui_end();
    }

    pid = fork_exec(args, fd);
    if (pid < 0) {
        error_msg("Error: %s", strerror(errno));
        close(p[1]);
        prompt = 0;
    } else {
        // Must close write end of the pipe before read_errors() or
        // the read end never gets EOF!
        close(p[1]);
        read_errors(c, p[0], quiet);
        handle_child_error(pid);
    }
    if (!quiet) {
        term_raw();
        if (prompt) {
            any_key();
        }
        resize();
        editor.child_controls_terminal = false;
    }
    close(p[0]);
    close(dev_null);
    close(fd[0]);
}

void spawn(char **args, int fd[3], bool prompt)
{
    int pid, quiet, redir_count = 0;
    int dev_null = open_dev_null(O_WRONLY);

    if (dev_null < 0) {
        return;
    }
    if (fd[0] < 0) {
        fd[0] = open_dev_null(O_RDONLY);
        if (fd[0] < 0) {
            close(dev_null);
            return;
        }
        redir_count++;
    }
    if (fd[1] < 0) {
        fd[1] = dev_null;
        redir_count++;
    }
    if (fd[2] < 0) {
        fd[2] = dev_null;
        redir_count++;
    }
    quiet = redir_count == 3;

    if (!quiet) {
        editor.child_controls_terminal = true;
        ui_end();
    }

    pid = fork_exec(args, fd);
    if (pid < 0) {
        error_msg("Error: %s", strerror(errno));
        prompt = false;
    } else {
        handle_child_error(pid);
    }
    if (!quiet) {
        term_raw();
        if (prompt) {
            any_key();
        }
        resize();
        editor.child_controls_terminal = false;
    }
    if (dev_null >= 0) {
        close(dev_null);
    }
}
