#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>
#include "log.h"
#include "array.h"
#include "debug.h"
#include "str-util.h"
#include "xreadwrite.h"
#include "xsnprintf.h"

// These are initialized during early startup and then never changed,
// so they're deemed an "acceptable" use of globals:
static char dim[] = "\033[2m";
static char sgr0[] = "\033[0m";
static LogLevel log_level = LOG_LEVEL_NONE;
static int logfd = -1;

static const char log_levels[][8] = {
    [LOG_LEVEL_NONE] = "none",
    [LOG_LEVEL_ERROR] = "error",
    [LOG_LEVEL_WARNING] = "warning",
    [LOG_LEVEL_INFO] = "info",
    [LOG_LEVEL_DEBUG] = "debug",
};

UNITTEST {
    CHECK_STRING_ARRAY(log_levels);
}

LogLevel log_level_from_str(const char *str)
{
    if (!str || str[0] == '\0') {
        // This is the default log level, which takes effect when
        // $DTE_LOG is set and $DTE_LOG_LEVEL is unset (or empty)
        return (DEBUG >= 2) ? LOG_LEVEL_DEBUG : LOG_LEVEL_INFO;
    }
    return STR_TO_ENUM(str, log_levels, LOG_LEVEL_NONE);
}

bool log_init(const char *filename, LogLevel level)
{
    BUG_ON(!filename);
    BUG_ON(level < LOG_LEVEL_NONE);
    BUG_ON(level > LOG_LEVEL_DEBUG);
    BUG_ON(logfd != -1);
    BUG_ON(log_level != LOG_LEVEL_NONE);

    if (level == LOG_LEVEL_NONE) {
        return true;
    }

    int flags = O_WRONLY | O_CREAT | O_APPEND | O_TRUNC | O_CLOEXEC;
    logfd = xopen(filename, flags, 0666);
    if (unlikely(logfd < 0 || xwrite_all(logfd, "\n", 1) != 1)) {
        return false;
    }

    if (!isatty(logfd)) {
        dim[0] = '\0';
        sgr0[0] = '\0';
    }

    log_level = level;
    if (DEBUG < 2 && level >= LOG_LEVEL_DEBUG) {
        const char *req = log_levels[level];
        static_assert(LOG_LEVEL_INFO == LOG_LEVEL_DEBUG - 1);
        log_level = LOG_LEVEL_INFO;
        LOG_WARNING("log level '%s' unavailable; falling back to 'info'", req);
    }

    LOG_INFO("logging to '%s' (level: %s)", filename, log_levels[log_level]);

    struct utsname u;
    if (likely(uname(&u) >= 0)) {
        LOG_INFO("system: %s/%s %s", u.sysname, u.machine, u.release);
    } else {
        LOG_ERROR("uname() failed: %s", strerror(errno));
    }
    return true;
}

void log_msgv(LogLevel level, const char *file, int line, const char *fmt, va_list ap)
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
