#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "debug.h"
#include "exitcode.h"
#include "xreadwrite.h"
#include "xsnprintf.h"

static void no_op(void) {}
static void (*cleanup_handler)(void) = no_op;

void set_fatal_error_cleanup_handler(void (*handler)(void))
{
    cleanup_handler = handler;
}

noreturn void fatal_error(const char *msg, int err)
{
    cleanup_handler();
    errno = err;
    perror(msg);
    abort();
}

#if DEBUG >= 1
noreturn
void bug(const char *file, int line, const char *func, const char *fmt, ...)
{
    cleanup_handler();
    fprintf(stderr, "\n%s:%d: **BUG** in %s() function: '", file, line, func);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputs("'\n", stderr);
    fflush(stderr);
    abort();
}
#endif

#if DEBUG >= 2
static int logfd = -1;

void log_init(const char *varname)
{
    const char *path = getenv(varname);
    if (!path || path[0] == '\0') {
        return;
    }
    logfd = xopen(path, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0666);
    if (logfd < 0) {
        const char *err = strerror(errno);
        fprintf(stderr, "Failed to open '%s' ($%s): %s\n", path, varname, err);
        exit(EX_IOERR);
    }
}

void debug_log(const char *function, const char *fmt, ...)
{
    if (logfd < 0) {
        return;
    }

    char buf[4096];
    size_t write_max = ARRAY_COUNT(buf) - 1;
    const size_t len1 = xsnprintf(buf, write_max, "%s: ", function);
    write_max -= len1;

    va_list ap;
    va_start(ap, fmt);
    const size_t len2 = xvsnprintf(buf + len1, write_max, fmt, ap);
    va_end(ap);

    size_t n = len1 + len2;
    buf[n++] = '\n';
    xwrite(logfd, buf, n);
}
#endif
