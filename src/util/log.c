#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "log.h"
#include "array.h"
#include "debug.h"
#include "str-util.h"
#include "xstdio.h"

// These are initialized during early startup and then never changed,
// so they're deemed an "acceptable" use of globals:
static char dim[] = "\033[2m";
static char sgr0[] = "\033[0m";
static LogLevel log_level = LOG_LEVEL_NONE;
static FILE *logfile = NULL;

static char log_colors[][8] = {
    [LOG_LEVEL_NONE] = "",
    [LOG_LEVEL_CRITICAL] = "\033[1;31m",
    [LOG_LEVEL_ERROR] = "\033[31m",
    [LOG_LEVEL_WARNING] = "\033[33m",
    [LOG_LEVEL_INFO] = "",
    [LOG_LEVEL_DEBUG] = "",
    [LOG_LEVEL_TRACE] = "",
};

static const char log_levels[][8] = {
    [LOG_LEVEL_NONE] = "none",
    [LOG_LEVEL_CRITICAL] = "crit",
    [LOG_LEVEL_ERROR] = "error",
    [LOG_LEVEL_WARNING] = "warning",
    [LOG_LEVEL_INFO] = "info",
    [LOG_LEVEL_DEBUG] = "debug",
    [LOG_LEVEL_TRACE] = "trace",
};

static const char log_prefixes[][8] = {
    [LOG_LEVEL_NONE] = "_",
    [LOG_LEVEL_CRITICAL] = "crit",
    [LOG_LEVEL_ERROR] = " err",
    [LOG_LEVEL_WARNING] = "warn",
    [LOG_LEVEL_INFO] = "info",
    [LOG_LEVEL_DEBUG] = " dbg",
    [LOG_LEVEL_TRACE] = "trce",
};

UNITTEST {
    CHECK_STRING_ARRAY(log_levels);
    CHECK_STRING_ARRAY(log_prefixes);
}

LogLevel log_level_default(void)
{
    return (DEBUG >= 2) ? LOG_LEVEL_DEBUG : LOG_LEVEL_INFO;
}

static LogLevel log_level_max(void)
{
    return (DEBUG >= 3) ? LOG_LEVEL_TRACE : log_level_default();
}

LogLevel log_level_from_str(const char *str)
{
    if (!str || str[0] == '\0') {
        return log_level_default();
    }
    return STR_TO_ENUM(str, log_levels, LOG_LEVEL_NONE);
}

const char *log_level_to_str(LogLevel level)
{
    BUG_ON(level < LOG_LEVEL_NONE);
    BUG_ON(level > LOG_LEVEL_TRACE);
    return log_levels[level];
}

LogLevel log_open(const char *filename, LogLevel level)
{
    BUG_ON(!filename);
    BUG_ON(level < LOG_LEVEL_NONE);
    BUG_ON(level > LOG_LEVEL_TRACE);
    BUG_ON(logfile);
    BUG_ON(log_level != LOG_LEVEL_NONE);

    if (level == LOG_LEVEL_NONE) {
        return LOG_LEVEL_NONE;
    }

    logfile = xfopen(filename, "w", O_APPEND | O_CLOEXEC, 0666);
    if (!logfile || fputc('\n', logfile) < 0 || fflush(logfile) != 0) {
        return LOG_LEVEL_NONE;
    }

    if (!isatty(fileno(logfile))) {
        dim[0] = '\0';
        sgr0[0] = '\0';
        memset(log_colors, '\0', sizeof(log_colors));
    }

    log_level = MIN(level, log_level_max());
    return log_level;
}

bool log_close(void)
{
    return log_level == LOG_LEVEL_NONE || fclose(logfile) == 0;
}

void log_msgv(LogLevel level, const char *file, int line, const char *fmt, va_list ap)
{
    BUG_ON(level <= LOG_LEVEL_NONE);
    BUG_ON(level > LOG_LEVEL_TRACE);
    if (level > log_level) {
        return;
    }

    BUG_ON(!logfile);
    const char *prefix = log_prefixes[level];
    const char *color = log_colors[level];
    const char *reset = color[0] ? sgr0 : "";
    xfprintf(logfile, "%s%s%s: %s%s:%d:%s ", color, prefix, reset, dim, file, line, sgr0);
    xvfprintf(logfile, fmt, ap);
    xfputc('\n', logfile);
}

void log_msg(LogLevel level, const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_msgv(level, file, line, fmt, ap);
    va_end(ap);
}
