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
#include "terminal/mode.h"
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
            perror_msg("select");
            break;
        }

        if (FD_ISSET(rfd, &rfds)) {
            char data[8192];

            ssize_t rc = read(rfd, data, sizeof(data));
            if (rc < 0) {
                perror_msg("read");
                break;
            }
            if (!rc) {
                if (wlen < fdata->in_len) {
                    error_msg("Command did not read all data.");
                }
                break;
            }
            string_append_buf(&buf, data, (size_t) rc);
        }
        if (wfdsp && FD_ISSET(wfd, &wfds)) {
            ssize_t rc = write(wfd, fdata->in + wlen, fdata->in_len - wlen);
            if (rc < 0) {
                perror_msg("write");
                break;
            }
            wlen += (size_t) rc;
            if (wlen == fdata->in_len) {
                if (close(wfd)) {
                    perror_msg("close");
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
    int fd = open("/dev/null", flags | O_CLOEXEC);
    if (fd < 0) {
        error_msg("Error opening /dev/null: %s", strerror(errno));
    }
    return fd;
}

static int handle_child_error(pid_t pid)
{
    int ret = wait_child(pid);
    if (ret < 0) {
        perror_msg("waitpid");
    } else if (ret >= 256) {
        error_msg("Child received signal %d", ret >> 8);
    } else if (ret) {
        error_msg("Child returned %d", ret);
    }
    return ret;
}

static void yield_terminal(void)
{
    editor.child_controls_terminal = true;
    editor.ui_end();
}

static void resume_terminal(bool prompt)
{
    term_raw();
    if (prompt) {
        any_key();
    }
    editor.resize();
    editor.child_controls_terminal = false;
}

bool spawn_filter(char **argv, FilterData *data)
{
    int p0[2] = {-1, -1};
    int p1[2] = {-1, -1};
    int dev_null = -1;

    data->out = NULL;
    data->out_len = 0;

    if (!pipe_close_on_exec(p0) || !pipe_close_on_exec(p1)) {
        perror_msg("pipe");
        goto error;
    }
    dev_null = open_dev_null(O_WRONLY);
    if (dev_null < 0) {
        goto error;
    }

    int fd[3] = {p0[0], p1[1], dev_null};
    const pid_t pid = fork_exec(argv, fd);
    if (pid < 0) {
        perror_msg("fork_exec");
        goto error;
    }

    close(dev_null);
    close(p0[0]);
    close(p1[1]);
    filter(p1[0], p0[1], data);
    close(p1[0]);
    close(p0[1]);

    if (handle_child_error(pid)) {
        free(data->out);
        data->out = NULL;
        data->out_len = 0;
        return false;
    }
    return true;
error:
    close(p0[0]);
    close(p0[1]);
    close(p1[0]);
    close(p1[1]);
    close(dev_null);
    return false;
}

bool spawn_source(char **argv, String *output)
{
    int p[2] = {-1, -1};
    int dev_null_r = -1;
    int dev_null_w = -1;
    if (!pipe_close_on_exec(p)) {
        perror_msg("pipe");
        goto error;
    }
    dev_null_r = open_dev_null(O_RDONLY);
    if (dev_null_r < 0) {
        goto error;
    }
    dev_null_w = open_dev_null(O_WRONLY);
    if (dev_null_w < 0) {
        goto error;
    }

    int fd[3] = {dev_null_r, p[1], dev_null_w};
    const pid_t pid = fork_exec(argv, fd);
    if (pid < 0) {
        perror_msg("fork_exec");
        goto error;
    }
    close(dev_null_r);
    close(dev_null_w);
    close(p[1]);

    while (1) {
        char buf[8192];
        ssize_t rc = xread(p[0], buf, sizeof(buf));
        if (unlikely(rc < 0)) {
            perror_msg("read");
            close(p[0]);
            string_free(output);
            return false;
        }
        if (rc == 0) {
            break;
        }
        string_append_buf(output, buf, (size_t) rc);
    }

    close(p[0]);
    if (handle_child_error(pid)) {
        string_free(output);
        return false;
    }
    return true;

error:
    close(p[0]);
    close(p[1]);
    close(dev_null_r);
    close(dev_null_w);
    return false;
}

bool spawn_sink(char **argv, const char *text, size_t length)
{
    int p[2] = {-1, -1};
    int dev_null = -1;
    if (!pipe_close_on_exec(p)) {
        perror_msg("pipe");
        goto error;
    }
    dev_null = open_dev_null(O_WRONLY);
    if (dev_null < 0) {
        goto error;
    }

    int fd[3] = {p[0], dev_null, dev_null};
    const pid_t pid = fork_exec(argv, fd);
    if (pid < 0) {
        perror_msg("fork_exec");
        goto error;
    }

    close(dev_null);
    close(p[0]);
    if (length && xwrite(p[1], text, length) < 0) {
        perror_msg("write");
        close(p[1]);
        return false;
    }
    close(p[1]);
    return !handle_child_error(pid);

error:
    close(p[0]);
    close(p[1]);
    close(dev_null);
    return false;
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
    if (!pipe_close_on_exec(p)) {
        perror_msg("pipe");
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
        yield_terminal();
    }

    const pid_t pid = fork_exec(args, fd);
    if (pid < 0) {
        perror_msg("fork_exec");
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
        resume_terminal(prompt);
    }
    close(p[0]);
    close(dev_null);
    close(fd[0]);
}

void spawn(char **args, bool quiet, bool prompt)
{
    int fd[3] = {0, 1, 2};
    if (quiet) {
        if ((fd[0] = open_dev_null(O_RDONLY)) < 0) {
            return;
        }
        if ((fd[1] = open_dev_null(O_WRONLY)) < 0) {
            close(fd[0]);
            return;
        }
        fd[2] = fd[1];
    } else {
        yield_terminal();
    }

    const pid_t pid = fork_exec(args, fd);
    if (pid < 0) {
        perror_msg("fork_exec");
        prompt = false;
    } else {
        handle_child_error(pid);
    }

    if (quiet) {
        close(fd[0]);
        close(fd[1]);
    } else {
        resume_terminal(prompt);
    }
}
