#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include "spawn.h"
#include "editor.h"
#include "error.h"
#include "msg.h"
#include "regexp.h"
#include "terminal/terminal.h"
#include "util/exec.h"
#include "util/string.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"

static void handle_error_msg(const Compiler *c, char *str)
{
    size_t i, len;

    for (i = 0; str[i]; i++) {
        if (str[i] == '\n') {
            str[i] = '\0';
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
            if (p->file_idx >= 0) {
                msg->loc->filename = xstrdup(m.ptrs[p->file_idx]);
                if (p->line_idx >= 0) {
                    str_to_ulong(m.ptrs[p->line_idx], &msg->loc->line);
                }
                if (p->column_idx >= 0) {
                    str_to_ulong(m.ptrs[p->column_idx], &msg->loc->column);
                }
            }
            add_message(msg);
        }
        ptr_array_free(&m);
        return;
    }
    add_message(new_message(str));
}

static void read_errors(const Compiler *c, int fd, bool quiet)
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

static int handle_child_error(pid_t pid)
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

    int fd[3] = {p0[0], p1[1], dev_null};
    const pid_t pid = fork_exec(argv, fd);
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

int spawn_writer(char **argv, const char *text, size_t length)
{
    if (length == 0) {
        return 0;
    }

    int p[2] = {-1, -1};
    int dev_null = -1;
    if (pipe_close_on_exec(p)) {
        error_msg("pipe: %s", strerror(errno));
        goto error;
    }
    dev_null = open_dev_null(O_WRONLY);
    if (dev_null < 0) {
        goto error;
    }

    int fd[3] = {p[0], dev_null, dev_null};
    const pid_t pid = fork_exec(argv, fd);
    if (pid < 0) {
        error_msg("Error: %s", strerror(errno));
        goto error;
    }

    close(dev_null);
    close(p[0]);
    if (xwrite(p[1], text, length) < 0) {
        error_msg("write: %s", strerror(errno));
        close(p[1]);
        return -1;
    }
    close(p[1]);
    return handle_child_error(pid) ? -1 : 0;

error:
    close(p[0]);
    close(p[1]);
    close(dev_null);
    return -1;
}

void spawn_compiler(char **args, SpawnFlags flags, const Compiler *c)
{
    const bool read_stdout = !!(flags & SPAWN_READ_STDOUT);
    const bool quiet = !!(flags & SPAWN_QUIET);
    bool prompt = !!(flags & SPAWN_PROMPT);
    int p[2], fd[3];

    fd[0] = open_dev_null(O_RDONLY);
    if (fd[0] < 0) {
        return;
    }
    const int dev_null = open_dev_null(O_WRONLY);
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
        editor.ui_end();
    }

    const pid_t pid = fork_exec(args, fd);
    if (pid < 0) {
        error_msg("Error: %s", strerror(errno));
        close(p[1]);
        prompt = false;
    } else {
        // Must close write end of the pipe before read_errors() or
        // the read end never gets EOF!
        close(p[1]);
        read_errors(c, p[0], quiet);
        handle_child_error(pid);
    }
    if (!quiet) {
        terminal.raw();
        if (prompt) {
            any_key();
        }
        editor.resize();
        editor.child_controls_terminal = false;
    }
    close(p[0]);
    close(dev_null);
    close(fd[0]);
}

void spawn(char **args, int fd[3], bool prompt)
{
    const int dev_null = open_dev_null(O_WRONLY);
    if (dev_null < 0) {
        return;
    }

    unsigned int redir_count = 0;
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

    const bool quiet = (redir_count == 3);
    if (!quiet) {
        editor.child_controls_terminal = true;
        editor.ui_end();
    }

    const pid_t pid = fork_exec(args, fd);
    if (pid < 0) {
        error_msg("Error: %s", strerror(errno));
        prompt = false;
    } else {
        handle_child_error(pid);
    }
    if (!quiet) {
        terminal.raw();
        if (prompt) {
            any_key();
        }
        editor.resize();
        editor.child_controls_terminal = false;
    }
    if (dev_null >= 0) {
        close(dev_null);
    }
}
