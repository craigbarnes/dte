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
// NOLINTBEGIN(*-avoid-non-const-global-variables)
static ErrorBuffer err;
// NOLINTEND(*-avoid-non-const-global-variables)

void clear_error(void)
{
    err.buf[0] = '\0';
}

bool error_msg(const char *format, ...)
{
    const char *cmd = current_command ? current_command->name : NULL;
    const char *file = current_config.file;
    const unsigned int line = current_config.line;
    const size_t size = sizeof(err.buf);
    int pos = 0;

    if (file && cmd) {
        pos = snprintf(err.buf, size, "%s:%u: %s: ", file, line, cmd);
    } else if (file) {
        pos = snprintf(err.buf, size, "%s:%u: ", file, line);
    } else if (cmd) {
        pos = snprintf(err.buf, size, "%s: ", cmd);
    }

    if (unlikely(pos < 0)) {
        // Note: POSIX snprintf(3) *does* set errno on failure (unlike ISO C)
        LOG_ERRNO("snprintf");
        pos = 0;
    }

    if (likely(pos < (size - 3))) {
        va_list ap;
        va_start(ap, format);
        vsnprintf(err.buf + pos, size - pos, format, ap);
        va_end(ap);
    } else {
        LOG_WARNING("no buffer space left for error message");
    }

    err.is_error = true;
    err.nr_errors++;

    if (err.print_to_stderr) {
        xfputs(err.buf, stderr);
        xfputc('\n', stderr);
    }

    LOG_INFO("%s", err.buf);

    // Always return false, to allow tail-calling as `return error_msg(...);`
    // from command handlers, instead of `error_msg(...); return false;`
    return false;
}

bool error_msg_errno(const char *prefix)
{
    return error_msg("%s: %s", prefix, strerror(errno));
}

void info_msg(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsnprintf(err.buf, sizeof(err.buf), format, ap);
    va_end(ap);
    err.is_error = false;
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
