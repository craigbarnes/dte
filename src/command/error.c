#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "error.h"
#include "command/run.h"
#include "util/log.h"
#include "util/xstdio.h"

VPRINTF(2)
static void error_msgv(ErrorBuffer *eb, const char *format, va_list ap)
{
    const size_t size = sizeof(eb->buf);
    const char *cmd = eb->command_name;
    const char *file = eb->config_filename;
    unsigned int line  = eb->config_line;
    int pos = 0;

    if (file && cmd) {
        pos = snprintf(eb->buf, size, "%s:%u: %s: ", file, line, cmd);
    } else if (file) {
        pos = snprintf(eb->buf, size, "%s:%u: ", file, line);
    } else if (cmd) {
        pos = snprintf(eb->buf, size, "%s: ", cmd);
    }

    if (unlikely(pos < 0)) {
        // Note: POSIX snprintf(3) *does* set errno on failure (unlike ISO C)
        LOG_ERRNO("snprintf");
        pos = 0;
    }

    if (likely(pos < (size - 3))) {
        vsnprintf(eb->buf + pos, size - pos, format, ap);
    } else {
        LOG_WARNING("no buffer space left for error message");
    }

    if (eb->print_to_stderr) {
        xfputs(eb->buf, stderr);
        xfputc('\n', stderr);
    }

    eb->is_error = true;
    eb->nr_errors++;
    LOG_INFO("%s", eb->buf);
}

bool error_msg(ErrorBuffer *eb, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    error_msgv(eb, format, ap);
    va_end(ap);

    // Always return false, to allow tail-calling as `return error_msg(...);`
    // from command handlers, instead of `error_msg(...); return false;`
    return false;
}

bool error_msg_for_cmd(ErrorBuffer *eb, const char *cmd, const char *format, ...)
{
    const char *saved_cmd = eb->command_name;
    eb->command_name = cmd;

    va_list ap;
    va_start(ap, format);
    error_msgv(eb, format, ap);
    va_end(ap);

    eb->command_name = saved_cmd;
    return false;
}

bool error_msg_errno(ErrorBuffer *eb, const char *prefix)
{
    return error_msg(eb, "%s: %s", prefix, strerror(errno));
}

bool info_msg(ErrorBuffer *eb, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsnprintf(eb->buf, sizeof(eb->buf), format, ap);
    va_end(ap);
    eb->is_error = false;
    return true; // To allow tail-calling from command handlers
}

void clear_error(ErrorBuffer *eb)
{
    eb->buf[0] = '\0';
}
