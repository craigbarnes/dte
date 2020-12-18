#include <errno.h>
#include <stddef.h>
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
#include "util/exec.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"

static void handle_error_msg(const Compiler *c, char *str)
{
    if (str[0] == '\0' || str[0] == '\n') {
        return;
    }

    size_t str_len = 0;
    for (char ch; (ch = str[str_len]); str_len++) {
        if (ch == '\t') {
            str[str_len] = ' ';
        }
    }
    if (str[str_len - 1] == '\n') {
        str[--str_len] = '\0';
    }

    for (size_t i = 0, n = c->error_formats.count; i < n; i++) {
        const ErrorFormat *p = c->error_formats.ptrs[i];
        regmatch_t m[16];
        if (!regexp_exec(&p->re, str, str_len, ARRAY_COUNT(m), m, 0)) {
            continue;
        }

        if (p->ignore) {
            return;
        }

        int8_t mi = p->msg_idx;
        if (m[mi].rm_so < 0) {
            mi = 0;
        }
        Message *msg = new_message(str + m[mi].rm_so, m[mi].rm_eo - m[mi].rm_so);
        msg->loc = xnew0(FileLocation, 1);

        int8_t fi = p->file_idx;
        if (fi >= 0 && m[fi].rm_so >= 0) {
            msg->loc->filename = xstrslice(str, m[fi].rm_so, m[fi].rm_eo);
            int8_t li = p->line_idx;
            if (li >= 0 && m[li].rm_so >= 0) {
                size_t len = m[li].rm_eo - m[li].rm_so;
                unsigned long val;
                size_t parsed_len = buf_parse_ulong(str + m[li].rm_so, len, &val);
                if (parsed_len == len) {
                    msg->loc->line = val;
                }
            }
            int8_t ci = p->column_idx;
            if (ci >= 0 && m[ci].rm_so >= 0) {
                size_t len = m[ci].rm_eo - m[ci].rm_so;
                unsigned long val;
                size_t parsed_len = buf_parse_ulong(str + m[ci].rm_so, len, &val);
                if (parsed_len == len) {
                    msg->loc->column = val;
                }
            }
        }

        add_message(msg);
        return;
    }

    add_message(new_message(str, str_len));
}

static void read_errors(const Compiler *c, int fd, bool quiet)
{
    FILE *f = fdopen(fd, "r");
    if (unlikely(!f)) {
        return;
    }
    char line[4096];
    while (fgets(line, sizeof(line), f)) {
        if (!quiet) {
            fputs(line, stderr);
        }
        handle_error_msg(c, line);
    }
    fclose(f);
}

static void filter(int rfd, int wfd, SpawnContext *ctx)
{
    size_t wlen = 0;
    if (!ctx->input.length) {
        xclose(wfd);
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
                if (wlen < ctx->input.length) {
                    error_msg("Command did not read all data.");
                }
                break;
            }
            string_append_buf(&ctx->output, data, (size_t) rc);
        }

        if (wfdsp && FD_ISSET(wfd, &wfds)) {
            ssize_t rc = write(wfd, ctx->input.data + wlen, ctx->input.length - wlen);
            if (rc < 0) {
                perror_msg("write");
                break;
            }
            wlen += (size_t) rc;
            if (wlen == ctx->input.length) {
                if (xclose(wfd)) {
                    perror_msg("close");
                    break;
                }
                wfd = -1;
            }
        }
    }
}

static int open_dev_null(int flags)
{
    int fd = xopen("/dev/null", flags | O_CLOEXEC, 0);
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

static void yield_terminal(bool quiet)
{
    if (quiet) {
        term_raw_isig();
    } else {
        editor.child_controls_terminal = true;
        ui_end();
    }
}

static void resume_terminal(bool quiet, bool prompt)
{
    term_raw();
    if (!quiet && editor.child_controls_terminal) {
        if (prompt) {
            any_key();
        }
        ui_start();
        editor.child_controls_terminal = false;
    }
}

static void exec_error(const char *argv0)
{
    error_msg("Unable to exec '%s': %s", argv0, strerror(errno));
}

bool spawn_filter(SpawnContext *ctx)
{
    int p0[2] = {-1, -1};
    int p1[2] = {-1, -1};
    int dev_null = -1;
    if (!pipe_close_on_exec(p0) || !pipe_close_on_exec(p1)) {
        perror_msg("pipe");
        goto error;
    }

    dev_null = open_dev_null(O_WRONLY);
    if (dev_null < 0) {
        goto error;
    }

    int fd[3] = {p0[0], p1[1], dev_null};
    term_raw_isig();
    const pid_t pid = fork_exec(ctx->argv, ctx->env, fd);
    if (pid < 0) {
        exec_error(ctx->argv[0]);
        goto error;
    }

    xclose(dev_null);
    xclose(p0[0]);
    xclose(p1[1]);
    filter(p1[0], p0[1], ctx);
    xclose(p1[0]);
    xclose(p0[1]);

    int err = handle_child_error(pid);
    term_raw();
    if (err) {
        string_free(&ctx->output);
        return false;
    }
    return true;

error:
    term_raw();
    xclose(p0[0]);
    xclose(p0[1]);
    xclose(p1[0]);
    xclose(p1[1]);
    xclose(dev_null);
    return false;
}

bool spawn_source(SpawnContext *ctx)
{
    bool quiet = !!(ctx->flags & SPAWN_QUIET);
    int p[2] = {-1, -1};
    int dev_null_r = -1;
    int dev_null_w = -1;

    if (!pipe_close_on_exec(p)) {
        perror_msg("pipe");
        return false;
    }

    int fd[3] = {0, p[1], 2};
    if (quiet) {
        dev_null_r = open_dev_null(O_RDONLY);
        if (dev_null_r < 0) {
            goto error;
        }
        fd[0] = dev_null_r;
        dev_null_w = open_dev_null(O_WRONLY);
        if (dev_null_w < 0) {
            goto error;
        }
        fd[2] = dev_null_w;
    }

    yield_terminal(quiet);
    const pid_t pid = fork_exec(ctx->argv, ctx->env, fd);
    if (pid < 0) {
        exec_error(ctx->argv[0]);
        goto error;
    }

    if (quiet) {
        xclose(dev_null_r);
        xclose(dev_null_w);
        dev_null_r = dev_null_w = -1;
    }
    xclose(p[1]);
    p[1] = -1;

    while (1) {
        char buf[8192];
        ssize_t rc = xread(p[0], buf, sizeof(buf));
        if (unlikely(rc < 0)) {
            perror_msg("read");
            goto error;
        }
        if (rc == 0) {
            break;
        }
        string_append_buf(&ctx->output, buf, (size_t) rc);
    }

    xclose(p[0]);
    p[0] = -1;
    if (handle_child_error(pid)) {
        goto error;
    }

    resume_terminal(quiet, false);
    return true;

error:
    string_free(&ctx->output);
    xclose(p[0]);
    xclose(p[1]);
    if (quiet) {
        xclose(dev_null_r);
        xclose(dev_null_w);
    }
    resume_terminal(quiet, false);
    return false;
}

bool spawn_sink(SpawnContext *ctx)
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
    term_raw_isig();
    const pid_t pid = fork_exec(ctx->argv, ctx->env, fd);
    if (pid < 0) {
        exec_error(ctx->argv[0]);
        goto error;
    }

    xclose(dev_null);
    xclose(p[0]);
    p[0] = dev_null = -1;
    size_t input_len = ctx->input.length;
    if (input_len && xwrite(p[1], ctx->input.data, input_len) < 0) {
        perror_msg("write");
        goto error;
    }
    xclose(p[1]);
    int err = handle_child_error(pid);
    term_raw();
    return !err;

error:
    term_raw();
    xclose(p[0]);
    xclose(p[1]);
    xclose(dev_null);
    return false;
}

void spawn_compiler(char **args, SpawnFlags flags, const Compiler *c)
{
    int fd[3];
    fd[0] = open_dev_null(O_RDONLY);
    if (fd[0] < 0) {
        return;
    }

    const int dev_null = open_dev_null(O_WRONLY);
    if (dev_null < 0) {
        xclose(fd[0]);
        return;
    }

    int p[2];
    if (!pipe_close_on_exec(p)) {
        perror_msg("pipe");
        xclose(dev_null);
        xclose(fd[0]);
        return;
    }

    const bool read_stdout = !!(flags & SPAWN_READ_STDOUT);
    const bool quiet = !!(flags & SPAWN_QUIET);
    bool prompt = !!(flags & SPAWN_PROMPT);
    if (read_stdout) {
        fd[1] = p[1];
        fd[2] = quiet ? dev_null : 2;
    } else {
        fd[1] = quiet ? dev_null : 1;
        fd[2] = p[1];
    }

    yield_terminal(quiet);
    const pid_t pid = fork_exec(args, NULL, fd);
    if (pid < 0) {
        exec_error(args[0]);
        xclose(p[1]);
        prompt = false;
    } else {
        // Must close write end of the pipe before read_errors() or
        // the read end never gets EOF!
        xclose(p[1]);
        read_errors(c, p[0], quiet);
        handle_child_error(pid);
    }
    resume_terminal(quiet, prompt);

    xclose(p[0]);
    xclose(dev_null);
    xclose(fd[0]);
}

void spawn(SpawnContext *ctx)
{
    bool quiet = !!(ctx->flags & SPAWN_QUIET);
    bool prompt = !!(ctx->flags & SPAWN_PROMPT);
    int fd[3] = {0, 1, 2};
    if (quiet) {
        if ((fd[0] = open_dev_null(O_RDONLY)) < 0) {
            return;
        }
        if ((fd[1] = open_dev_null(O_WRONLY)) < 0) {
            xclose(fd[0]);
            return;
        }
        fd[2] = fd[1];
    }

    yield_terminal(quiet);
    const pid_t pid = fork_exec(ctx->argv, ctx->env, fd);
    if (pid < 0) {
        exec_error(ctx->argv[0]);
        prompt = false;
    } else {
        handle_child_error(pid);
    }
    resume_terminal(quiet, prompt);

    if (quiet) {
        xclose(fd[0]);
        xclose(fd[1]);
    }
}
