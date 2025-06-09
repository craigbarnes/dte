#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "log.h"
#include "array.h"
#include "fd.h"
#include "xreadwrite.h"
#include "xstring.h"

// These are initialized during early startup and then never changed,
// so they're deemed an acceptable use of non-const globals:
// NOLINTBEGIN(*-avoid-non-const-global-variables)
static LogLevel log_level = LOG_LEVEL_NONE;
static int logfd = -1;
static char dim[] = "\033[2m";
static char sgr0[] = "\033[0m";
// NOLINTEND(*-avoid-non-const-global-variables)

static const struct llmap {
    char name[8];
    char prefix[5];
    char color[8];
} log_level_map[] = {
    [LOG_LEVEL_NONE] = {"none", "_", ""},
    [LOG_LEVEL_CRITICAL] = {"crit", "crit", "\033[1;31m"},
    [LOG_LEVEL_ERROR] = {"error", " err", "\033[31m"},
    [LOG_LEVEL_WARNING] = {"warning", "warn", "\033[33m"},
    [LOG_LEVEL_NOTICE] = {"notice", "note", "\033[34m"},
    [LOG_LEVEL_INFO] = {"info", "info", ""},
    [LOG_LEVEL_DEBUG] = {"debug", "dbug", ""},
    [LOG_LEVEL_TRACE] = {"trace", "trce", ""},
};

UNITTEST {
    CHECK_STRUCT_ARRAY(log_level_map, name);
    for (size_t i = 0; i < ARRAYLEN(log_level_map); i++) {
        const struct llmap *m = &log_level_map[i];
        BUG_ON(m->prefix[sizeof(m->prefix) - 1] != '\0');
        BUG_ON(m->color[sizeof(m->color) - 1] != '\0');
    }
}

LogLevel log_level_default(void)
{
    return DEBUG_LOGGING_ENABLED ? LOG_LEVEL_DEBUG : LOG_LEVEL_INFO;
}

static LogLevel log_level_max(void)
{
    return TRACE_LOGGING_ENABLED ? LOG_LEVEL_TRACE : log_level_default();
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
    return LOG_LEVEL_INVALID;
}

const char *log_level_to_str(LogLevel level)
{
    BUG_ON(level < LOG_LEVEL_NONE);
    BUG_ON(level > LOG_LEVEL_TRACE);
    return log_level_map[level].name;
}

LogLevel log_open(const char *filename, LogLevel level, bool use_color)
{
    BUG_ON(!filename);
    BUG_ON(level < LOG_LEVEL_NONE);
    BUG_ON(level > LOG_LEVEL_TRACE);
    BUG_ON(logfd >= 0);
    BUG_ON(log_level != LOG_LEVEL_NONE);

    if (level == LOG_LEVEL_NONE) {
        return LOG_LEVEL_NONE;
    }

    int oflags = O_WRONLY | O_CREAT | O_APPEND | O_TRUNC | O_CLOEXEC;
    int fd = xopen(filename, oflags, 0666);
    if (unlikely(fd < 0)) {
        return LOG_LEVEL_NONE;
    }

    bool ctty = is_controlling_tty(fd);
    if (unlikely(ctty || xwrite_all(fd, "\n", 1) != 1)) {
        int err = ctty ? EINVAL : errno;
        xclose(fd);
        errno = err;
        return LOG_LEVEL_NONE;
    }

    if (!use_color || !isatty(fd)) {
        dim[0] = '\0';
        sgr0[0] = '\0';
    }

    logfd = fd;
    log_level = MIN(level, log_level_max());
    return log_level;
}

bool log_close(void)
{
    return log_level == LOG_LEVEL_NONE || xclose(logfd) == 0;
}

bool log_level_enabled(LogLevel level)
{
    BUG_ON(level <= LOG_LEVEL_NONE || level > LOG_LEVEL_TRACE);
    BUG_ON(log_level < LOG_LEVEL_NONE || log_level > log_level_max());
    return level <= log_level;
}

// This function should be kept async signal safe
void log_write(LogLevel level, const char *str, size_t len)
{
    // log_level is a non-const global, but as noted above, it's never
    // modified after early startup and so poses no signal safety issues
    // (see https://austingroupbugs.net/view.php?id=728#c6430)
    if (!log_level_enabled(level)) {
        return;
    }

    // xwrite_all() only calls write(3), which is async signal safe
    // (see signal-safety(7))
    BUG_ON(logfd < 0);
    (void)!xwrite_all(logfd, str, len);
    (void)!xwrite_all(logfd, "\n", 1);
}

void log_msgv(LogLevel level, const char *file, int line, const char *fmt, va_list ap)
{
    if (!log_level_enabled(level)) {
        return;
    }

    BUG_ON(logfd < 0);
    char buf[2048];
    const size_t buf_size = sizeof(buf) - 1;
    const char *prefix = log_level_map[level].prefix;
    const char *color = sgr0[0] ? log_level_map[level].color : "";
    const char *reset = color[0] ? sgr0 : "";

    int len1 = snprintf (
        buf, buf_size,
        "%s%s%s: %s%s:%d:%s ",
        color, prefix, reset,
        dim, file, line, sgr0
    );

    if (unlikely(len1 < 0 || len1 >= buf_size)) {
        len1 = 0;
    }

    int len2 = vsnprintf(buf + len1, buf_size - len1, fmt, ap);
    size_t n = MIN(len1 + MAX(len2, 0), buf_size);
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
