#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>
#include "log.h"
#include "debug.h"
#include "exitcode.h"
#include "str-util.h"
#include "xreadwrite.h"
#include "xsnprintf.h"

static const char *dim = "";
static const char *sgr0 = "";
static LogLevel log_level = LOG_LEVEL_NONE;
static int logfd = -1;

static const char levels[][8] = {
    [LOG_LEVEL_NONE] = "none",
    [LOG_LEVEL_ERROR] = "error",
    [LOG_LEVEL_WARNING] = "warning",
    [LOG_LEVEL_INFO] = "info",
    [LOG_LEVEL_DEBUG] = "debug",
};

LogLevel log_level_from_str(const char *str)
{
    if (!str || str[0] == '\0') {
        // This is the default log level, which takes effect when
        // $DTE_LOG is set and $DTE_LOG_LEVEL is unset (or empty)
        return LOG_LEVEL_INFO;
    }

    for (size_t i = 0; i < ARRAYLEN(levels); i++) {
        if (streq(str, levels[i])) {
            return (LogLevel)i;
        }
    }

    return LOG_LEVEL_NONE;
}

void log_init(const char *filename, LogLevel level)
{
    BUG_ON(!filename);
    BUG_ON(level < LOG_LEVEL_NONE);
    BUG_ON(level > LOG_LEVEL_DEBUG);
    BUG_ON(logfd != -1);
    BUG_ON(log_level != LOG_LEVEL_NONE);

    if (level == LOG_LEVEL_NONE) {
        return;
    }

    logfd = xopen(filename, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0666);
    if (unlikely(logfd < 0)) {
        const char *err = strerror(errno);
        fprintf(stderr, "Failed to open log file '%s': %s\n", filename, err);
        exit(EX_IOERR);
    }
    if (unlikely(xwrite_all(logfd, "\n", 1) != 1)) {
        fprintf(stderr, "Failed to write to log: %s\n", strerror(errno));
        exit(EX_IOERR);
    }

    if (isatty(logfd)) {
        dim = "\033[2m";
        sgr0 = "\033[0m";
    }

    log_level = level;
    struct utsname u;
    if (likely(uname(&u) >= 0)) {
        LOG_INFO("system: %s/%s %s", u.sysname, u.machine, u.release);
    } else {
        LOG_ERROR("uname() failed: %s", strerror(errno));
    }
}

VPRINTF(4)
static void log_msgv(LogLevel level, const char *file, int line, const char *fmt, va_list ap)
{
    BUG_ON(level <= LOG_LEVEL_NONE);
    BUG_ON(level > LOG_LEVEL_DEBUG);
    if (level > log_level) {
        return;
    }

    BUG_ON(logfd < 0);
    char buf[4096];
    size_t write_max = ARRAYLEN(buf) - 1;
    const size_t len1 = xsnprintf(buf, write_max, "%s%s:%d:%s ", dim, file, line, sgr0);
    write_max -= len1;
    const size_t len2 = xvsnprintf(buf + len1, write_max, fmt, ap);
    size_t n = len1 + len2;
    buf[n++] = '\n';
    (void)!xwrite_all(logfd, buf, n);
}

void log_msg(LogLevel level, const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_msgv(level, file, line, fmt, ap);
    va_end(ap);
}
