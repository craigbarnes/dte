#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "error.h"
#include "command/run.h"
#include "config.h"
#include "util/log.h"
#include "util/xstdio.h"

static char error_buf[512];
static unsigned int nr_errors;
static bool msg_is_error;
static bool print_errors_to_stderr;

void clear_error(void)
{
    error_buf[0] = '\0';
}

bool error_msg(const char *format, ...)
{
    const char *cmd = current_command ? current_command->name : NULL;
    const char *file = current_config.file;
    const unsigned int line = current_config.line;
    const size_t size = sizeof(error_buf);
    int pos = 0;

    if (file && cmd) {
        pos = snprintf(error_buf, size, "%s:%u: %s: ", file, line, cmd);
    } else if (file) {
        pos = snprintf(error_buf, size, "%s:%u: ", file, line);
    } else if (cmd) {
        pos = snprintf(error_buf, size, "%s: ", cmd);
    }

    if (unlikely(pos < 0)) {
        // Note: POSIX snprintf(3) *does* set errno on failure (unlike ISO C)
        LOG_ERRNO("snprintf");
        pos = 0;
    }

    if (likely(pos < (size - 3))) {
        va_list ap;
        va_start(ap, format);
        vsnprintf(error_buf + pos, size - pos, format, ap);
        va_end(ap);
    } else {
        LOG_WARNING("no buffer space left for error message");
    }

    msg_is_error = true;
    nr_errors++;

    if (print_errors_to_stderr) {
        xfputs(error_buf, stderr);
        xfputc('\n', stderr);
    }

    LOG_INFO("%s", error_buf);

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
    vsnprintf(error_buf, sizeof(error_buf), format, ap);
    va_end(ap);
    msg_is_error = false;
}

const char *get_msg(bool *is_error)
{
    *is_error = msg_is_error;
    return error_buf;
}

unsigned int get_nr_errors(void)
{
    return nr_errors;
}

void set_print_errors_to_stderr(bool enable)
{
    print_errors_to_stderr = enable;
}
