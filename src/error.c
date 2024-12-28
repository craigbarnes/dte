#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "error.h"
#include "command/run.h"
#include "config.h"
#include "util/log.h"
#include "util/xstdio.h"

typedef struct {
    char buf[512];
    unsigned int nr_errors;
    bool is_error;
    bool print_to_stderr;
} ErrorBuffer;

// This is not accessed from signal handlers or multiple threads and
// is considered an acceptable use of non-const globals:
// NOLINTNEXTLINE(*-avoid-non-const-global-variables)
static ErrorBuffer err;

void clear_error(void)
{
    err.buf[0] = '\0';
}

VPRINTF(5)
static void error_msgv (
    ErrorBuffer *eb,
    const char *file,
    unsigned int line,
    const char *cmd,
    const char *format,
    va_list ap
) {
    const size_t size = sizeof(eb->buf);
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

bool error_msg(const char *format, ...)
{
    const char *cmd = current_command ? current_command->name : NULL;
    va_list ap;
    va_start(ap, format);
    error_msgv(&err, current_config.file, current_config.line, cmd, format, ap);
    va_end(ap);

    // Always return false, to allow tail-calling as `return error_msg(...);`
    // from command handlers, instead of `error_msg(...); return false;`
    return false;
}

bool error_msg_for_cmd(const char *cmd, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    error_msgv(&err, current_config.file, current_config.line, cmd, format, ap);
    va_end(ap);
    return false;
}

bool error_msg_errno(const char *prefix)
{
    return error_msg("%s: %s", prefix, strerror(errno));
}

bool info_msg(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsnprintf(err.buf, sizeof(err.buf), format, ap);
    va_end(ap);
    err.is_error = false;
    return true; // To allow tail-calling from command handlers
}

const char *get_msg(bool *is_error)
{
    *is_error = err.is_error;
    return err.buf;
}

unsigned int get_nr_errors(void)
{
    return err.nr_errors;
}

void errors_to_stderr(bool enable)
{
    err.print_to_stderr = enable;
}
