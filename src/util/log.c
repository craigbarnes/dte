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
static LogLevel log_level = LOG_LEVEL_NONE;
static FILE *logfile = NULL;
static bool log_colors = false;

static const char dim[] = "\033[2m";
static const char sgr0[] = "\033[0m";

static const struct {
    char name[8];
    char log_prefix[5];
    char color[8];
} log_level_map[] = {
    [LOG_LEVEL_NONE] = {"none", "_", ""},
    [LOG_LEVEL_CRITICAL] = {"crit", "crit", "\033[1;31m"},
    [LOG_LEVEL_ERROR] = {"error", " err", "\033[31m"},
    [LOG_LEVEL_WARNING] = {"warning", "warn", "\033[33m"},
    [LOG_LEVEL_INFO] = {"info", "info", ""},
    [LOG_LEVEL_DEBUG] = {"debug", " dbg", ""},
    [LOG_LEVEL_TRACE] = {"trace", "trce", ""},
};

UNITTEST {
    CHECK_STRUCT_ARRAY(log_level_map, name);
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
    for (LogLevel i = 0; i < ARRAYLEN(log_level_map); i++) {
        if (streq(str, log_level_map[i].name)) {
            return i;
        }
    }
    return LOG_LEVEL_NONE;
}

const char *log_level_to_str(LogLevel level)
{
    BUG_ON(level < LOG_LEVEL_NONE);
    BUG_ON(level > LOG_LEVEL_TRACE);
    return log_level_map[level].name;
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

    if (isatty(fileno(logfile))) {
        log_colors = true;
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
    const char *prefix = log_level_map[level].log_prefix;

    if (log_colors) {
        const char *color = log_level_map[level].color;
        const char *reset = color[0] ? sgr0 : "";
        xfprintf (
            logfile,
            "%s%s%s: %s%s:%d:%s ",
            color, prefix, reset,
            dim, file, line, sgr0
        );
    } else {
        xfprintf(logfile, "%s: %s:%d: ", prefix, file, line);
    }

    xvfprintf(logfile, fmt, ap);
    xfputc('\n', logfile);
    xfflush(logfile);
}

void log_msg(LogLevel level, const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_msgv(level, file, line, fmt, ap);
    va_end(ap);
}
