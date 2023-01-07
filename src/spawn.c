#include <errno.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "spawn.h"
#include "error.h"
#include "regexp.h"
#include "terminal/mode.h"
#include "util/debug.h"
#include "util/fd.h"
#include "util/fork-exec.h"
#include "util/ptr-array.h"
#include "util/str-util.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"
#include "util/xstdio.h"

static void handle_error_msg(const Compiler *c, MessageArray *msgs, char *str)
{
    if (str[0] == '\0' || str[0] == '\n') {
        return;
    }

    size_t str_len = str_replace_byte(str, '\t', ' ');
    if (str[str_len - 1] == '\n') {
        str[--str_len] = '\0';
    }

    for (size_t i = 0, n = c->error_formats.count; i < n; i++) {
        const ErrorFormat *p = c->error_formats.ptrs[i];
        regmatch_t m[ERRORFMT_CAPTURE_MAX];
        if (!regexp_exec(&p->re, str, str_len, ARRAYLEN(m), m, 0)) {
            continue;
        }
        if (p->ignore) {
            return;
        }

        int8_t mi = p->capture_index[ERRFMT_MESSAGE];
        if (m[mi].rm_so < 0) {
            mi = 0;
        }

        Message *msg = new_message(str + m[mi].rm_so, m[mi].rm_eo - m[mi].rm_so);
        msg->loc = xnew0(FileLocation, 1);

        int8_t fi = p->capture_index[ERRFMT_FILE];
        if (fi >= 0 && m[fi].rm_so >= 0) {
            msg->loc->filename = xstrslice(str, m[fi].rm_so, m[fi].rm_eo);

            unsigned long *const ptrs[] = {
                [ERRFMT_LINE] = &msg->loc->line,
                [ERRFMT_COLUMN] = &msg->loc->column,
            };

            static_assert(ARRAYLEN(ptrs) == 3);
            static_assert(ERRFMT_LINE == 1);
            static_assert(ERRFMT_COLUMN == 2);

            for (size_t j = ERRFMT_LINE; j < ARRAYLEN(ptrs); j++) {
                int8_t ci = p->capture_index[j];
                if (ci >= 0 && m[ci].rm_so >= 0) {
                    size_t len = m[ci].rm_eo - m[ci].rm_so;
                    unsigned long val;
                    if (len == buf_parse_ulong(str + m[ci].rm_so, len, &val)) {
                        *ptrs[j] = val;
                    }
                }
            }
        }

        add_message(msgs, msg);
        return;
    }

    add_message(msgs, new_message(str, str_len));
}

static void read_errors(const Compiler *c, MessageArray *msgs, int fd, bool quiet)
{
    FILE *f = fdopen(fd, "r");
    if (unlikely(!f)) {
        return;
    }
    char line[4096];
    while (xfgets(line, sizeof(line), f)) {
        if (!quiet) {
            xfputs(line, stderr);
        }
        handle_error_msg(c, msgs, line);
    }
    fclose(f);
}

static void handle_piped_data(int f[3], SpawnContext *ctx)
{
    BUG_ON(f[0] < 0 && f[1] < 0 && f[2] < 0);
    BUG_ON(f[0] >= 0 && f[0] <= 2);
    BUG_ON(f[1] >= 0 && f[1] <= 2);
    BUG_ON(f[2] >= 0 && f[2] <= 2);

    if (ctx->input.length == 0) {
        xclose(f[0]);
        f[0] = -1;
        if (f[1] < 0 && f[2] < 0) {
            return;
        }
    }

    struct pollfd fds[] = {
        {.fd = f[0], .events = POLLOUT},
        {.fd = f[1], .events = POLLIN},
        {.fd = f[2], .events = POLLIN},
    };

    size_t wlen = 0;
    while (1) {
        if (unlikely(poll(fds, ARRAYLEN(fds), -1) < 0)) {
            if (errno == EINTR) {
                continue;
            }
            error_msg_errno("poll");
            return;
        }

        for (size_t i = 0; i < ARRAYLEN(ctx->outputs); i++) {
            struct pollfd *pfd = fds + i + 1;
            if (pfd->revents & POLLIN) {
                String *output = &ctx->outputs[i];
                char *buf = string_reserve_space(output, 4096);
                ssize_t rc = xread(pfd->fd, buf, output->alloc - output->len);
                if (unlikely(rc < 0)) {
                    error_msg_errno("read");
                    return;
                }
                if (rc == 0) { // EOF
                    if (xclose(pfd->fd)) {
                        error_msg_errno("close");
                        return;
                    }
                    pfd->fd = -1;
                    continue;
                }
                output->len += rc;
            }
        }

        if (fds[0].revents & POLLOUT) {
            ssize_t rc = xwrite(fds[0].fd, ctx->input.data + wlen, ctx->input.length - wlen);
            if (unlikely(rc < 0)) {
                error_msg_errno("write");
                return;
            }
            wlen += (size_t) rc;
            if (wlen == ctx->input.length) {
                if (xclose(fds[0].fd)) {
                    error_msg_errno("close");
                    return;
                }
                fds[0].fd = -1;
            }
        }

        size_t active_fds = ARRAYLEN(fds);
        for (size_t i = 0; i < ARRAYLEN(fds); i++) {
            int rev = fds[i].revents;
            if (fds[i].fd < 0 || rev & POLLNVAL) {
                fds[i].fd = -1;
                active_fds--;
                continue;
            }
            if (rev & POLLERR || (rev & (POLLHUP | POLLIN)) == POLLHUP) {
                if (xclose(fds[i].fd)) {
                    error_msg_errno("close");
                }
                fds[i].fd = -1;
                active_fds--;
            }
        }
        if (active_fds == 0) {
            return;
        }
    }
}

static int open_dev_null(int flags)
{
    int fd = xopen("/dev/null", flags | O_CLOEXEC, 0);
    if (unlikely(fd < 0)) {
        error_msg_errno("Error opening /dev/null");
    }
    return fd;
}

static int handle_child_error(pid_t pid)
{
    int ret = wait_child(pid);
    if (ret < 0) {
        error_msg_errno("waitpid");
    } else if (ret >= 256) {
        int sig = ret >> 8;
        const char *str = strsignal(sig);
        error_msg("Child received signal %d (%s)", sig, str ? str : "??");
    } else if (ret) {
        error_msg("Child returned %d", ret);
    }
    return ret;
}

static void yield_terminal(EditorState *e, bool quiet)
{
    if (quiet) {
        term_raw_isig();
    } else {
        e->child_controls_terminal = true;
        ui_end(e);
    }
}

static void resume_terminal(EditorState *e, bool quiet, bool prompt)
{
    term_raw();
    if (!quiet && e->child_controls_terminal) {
        if (prompt) {
            any_key(&e->terminal, e->options.esc_timeout);
        }
        ui_start(e);
        e->child_controls_terminal = false;
    }
}

static void exec_error(const char *argv0)
{
    error_msg("Unable to exec '%s': %s", argv0, strerror(errno));
}

bool spawn_compiler(SpawnContext *ctx, const Compiler *c, MessageArray *msgs)
{
    BUG_ON(!ctx->editor);
    BUG_ON(!ctx->argv[0]);

    int fd[3];
    fd[0] = open_dev_null(O_RDONLY);
    if (fd[0] < 0) {
        return false;
    }

    int dev_null = open_dev_null(O_WRONLY);
    if (dev_null < 0) {
        xclose(fd[0]);
        return false;
    }

    int p[2];
    if (xpipe2(p, O_CLOEXEC) != 0) {
        error_msg_errno("pipe");
        xclose(dev_null);
        xclose(fd[0]);
        return false;
    }

    SpawnFlags flags = ctx->flags;
    bool read_stdout = !!(flags & SPAWN_READ_STDOUT);
    bool quiet = !!(flags & SPAWN_QUIET);
    bool prompt = !!(flags & SPAWN_PROMPT);
    if (read_stdout) {
        fd[1] = p[1];
        fd[2] = quiet ? dev_null : 2;
    } else {
        fd[1] = quiet ? dev_null : 1;
        fd[2] = p[1];
    }

    yield_terminal(ctx->editor, quiet);
    pid_t pid = fork_exec(ctx->argv, NULL, fd, quiet);
    if (pid == -1) {
        exec_error(ctx->argv[0]);
        xclose(p[1]);
        prompt = false;
    } else {
        // Must close write end of the pipe before read_errors() or
        // the read end never gets EOF!
        xclose(p[1]);
        read_errors(c, msgs, p[0], quiet);
        handle_child_error(pid);
    }
    resume_terminal(ctx->editor, quiet, prompt);

    xclose(p[0]);
    xclose(dev_null);
    xclose(fd[0]);
    return (pid != -1);
}

// Close fd only if valid (positive) and not stdin/stdout/stderr
static int safe_xclose(int fd)
{
    return (fd > STDERR_FILENO) ? xclose(fd) : 0;
}

static void safe_xclose_all(int fds[], size_t nr_fds)
{
    for (size_t i = 0; i < nr_fds; i++) {
        safe_xclose(fds[i]);
        fds[i] = -1;
    }
}

UNITTEST {
    int fds[] = {-2, -3, -4};
    safe_xclose_all(fds, 2);
    BUG_ON(fds[0] != -1);
    BUG_ON(fds[1] != -1);
    BUG_ON(fds[2] != -4);
    safe_xclose_all(fds, 3);
    BUG_ON(fds[2] != -1);
}

int spawn(SpawnContext *ctx)
{
    BUG_ON(!ctx->editor);
    BUG_ON(!ctx->argv[0]);

    int child_fds[3] = {-1, -1, -1};
    int parent_fds[3] = {-1, -1, -1};
    bool quiet = !!(ctx->flags & SPAWN_QUIET);
    size_t nr_pipes = 0;

    for (size_t i = 0; i < ARRAYLEN(child_fds); i++) {
        switch (ctx->actions[i]) {
        case SPAWN_TTY:
            if (!quiet) {
                child_fds[i] = i;
                break;
            }
            // Fallthrough
        case SPAWN_NULL:
            child_fds[i] = open_dev_null(O_RDWR);
            if (child_fds[i] < 0) {
                goto error_close;
            }
            break;
        case SPAWN_PIPE: {
            int p[2];
            if (xpipe2(p, O_CLOEXEC) != 0) {
                error_msg_errno("pipe");
                goto error_close;
            }
            BUG_ON(p[0] <= STDERR_FILENO);
            BUG_ON(p[1] <= STDERR_FILENO);
            child_fds[i] = i ? p[1] : p[0];
            parent_fds[i] = i ? p[0] : p[1];
            if (!fd_set_nonblock(parent_fds[i], true)) {
                error_msg_errno("fcntl");
                goto error_close;
            }
            nr_pipes++;
            break;
        }
        default:
            BUG("unhandled action type");
            goto error_close;
        }
    }

    yield_terminal(ctx->editor, quiet);
    pid_t pid = fork_exec(ctx->argv, ctx->env, child_fds, quiet);
    if (pid == -1) {
        exec_error(ctx->argv[0]);
        goto error_resume;
    }

    safe_xclose_all(child_fds, ARRAYLEN(child_fds));
    if (nr_pipes > 0) {
        handle_piped_data(parent_fds, ctx);
    }

    safe_xclose_all(parent_fds, ARRAYLEN(parent_fds));
    int err = wait_child(pid);
    if (err < 0) {
        error_msg_errno("waitpid");
    }

    resume_terminal(ctx->editor, quiet, !!(ctx->flags & SPAWN_PROMPT));
    return err;

error_resume:
    resume_terminal(ctx->editor, quiet, false);
error_close:
    safe_xclose_all(child_fds, ARRAYLEN(child_fds));
    safe_xclose_all(parent_fds, ARRAYLEN(parent_fds));
    return -1;
}
