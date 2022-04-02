#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>
#include "log.h"
#include "debug.h"
#include "exitcode.h"
#include "xreadwrite.h"
#include "xsnprintf.h"

#if DEBUG >= 2
static const char *dim = "";
static const char *sgr0 = "";
static int logfd = -1;

void log_init(const char *varname)
{
    BUG_ON(logfd != -1);
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
    if (xwrite_all(logfd, "\n", 1) != 1) {
        fprintf(stderr, "Failed to write to log: %s\n", strerror(errno));
        exit(EX_IOERR);
    }

    if (isatty(logfd)) {
        dim = "\033[2m";
        sgr0 = "\033[0m";
    }

    struct utsname u;
    if (uname(&u) >= 0) {
        DEBUG_LOG("system: %s/%s %s", u.sysname, u.machine, u.release);
    } else {
        DEBUG_LOG("uname() failed: %s", strerror(errno));
    }
}

VPRINTF(3)
static void debug_logv(const char *file, int line, const char *fmt, va_list ap)
{
    if (logfd < 0) {
        return;
    }
    char buf[4096];
    size_t write_max = ARRAYLEN(buf) - 1;
    const size_t len1 = xsnprintf(buf, write_max, "%s%s:%d:%s ", dim, file, line, sgr0);
    write_max -= len1;
    const size_t len2 = xvsnprintf(buf + len1, write_max, fmt, ap);
    size_t n = len1 + len2;
    buf[n++] = '\n';
    (void)!xwrite_all(logfd, buf, n);
}

void debug_log(const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    debug_logv(file, line, fmt, ap);
    va_end(ap);
}
#endif
