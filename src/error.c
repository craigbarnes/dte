#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "error.h"
#include "command/run.h"
#include "config.h"
#include "editor.h"
#include "util/debug.h"

static char error_buf[256];
static unsigned int nr_errors;
static bool msg_is_error;

void clear_error(void)
{
    error_buf[0] = '\0';
}

void error_msg(const char *format, ...)
{
    const char *cmd = current_command ? current_command->name : NULL;
    const char *file = current_config.file;
    const int line = current_config.line;
    const size_t size = sizeof(error_buf);
    int pos = 0;

    if (file && cmd) {
        pos = snprintf(error_buf, size, "%s:%d: %s: ", file, line, cmd);
    } else if (file) {
        pos = snprintf(error_buf, size, "%s:%d: ", file, line);
    } else if (cmd) {
        pos = snprintf(error_buf, size, "%s: ", cmd);
    }

    if (pos >= 0 && pos < (size - 3)) {
        va_list ap;
        va_start(ap, format);
        vsnprintf(error_buf + pos, size - pos, format, ap);
        va_end(ap);
    }

    msg_is_error = true;
    nr_errors++;

    if (editor.status != EDITOR_RUNNING) {
        fputs(error_buf, stderr);
        fputc('\n', stderr);
    }

    DEBUG_LOG("%s", error_buf);
}

void perror_msg(const char *prefix)
{
    error_msg("%s: %s", prefix, strerror(errno));
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
