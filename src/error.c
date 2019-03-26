#include <stdio.h>
#include "error.h"
#include "common.h"
#include "config.h"
#include "editor.h"
#include "util/xmalloc.h"

static char error_buf[256];
const char *const error_ptr = error_buf;
unsigned int nr_errors;
bool msg_is_error;

static Error *error_new(char *msg)
{
    Error *err = xnew0(Error, 1);
    err->msg = msg;
    return err;
}

Error *error_create(const char *format, ...)
{
    Error *err;
    va_list ap;

    va_start(ap, format);
    err = error_new(xvasprintf(format, ap));
    va_end(ap);
    return err;
}

Error *error_create_errno(int code, const char *format, ...)
{
    Error *err;
    va_list ap;

    va_start(ap, format);
    err = error_new(xvasprintf(format, ap));
    va_end(ap);
    err->code = code;
    return err;
}

void error_free(Error *err)
{
    if (err != NULL) {
        free(err->msg);
        free(err);
    }
}

void clear_error(void)
{
    error_buf[0] = '\0';
}

void error_msg(const char *format, ...)
{
    int pos = 0;

    if (config_file) {
        if (current_command) {
            pos = snprintf (
                error_buf,
                sizeof(error_buf),
                "%s:%d: %s: ",
                config_file,
                config_line,
                current_command->name
            );
        } else {
            pos = snprintf (
                error_buf,
                sizeof(error_buf),
                "%s:%d: ",
                config_file,
                config_line
            );
        }
    }

    if (pos >= 0 && pos < (sizeof(error_buf) - 3)) {
        va_list ap;
        va_start(ap, format);
        vsnprintf(error_buf + pos, sizeof(error_buf) - pos, format, ap);
        va_end(ap);
    }

    msg_is_error = true;
    nr_errors++;

    if (editor.status != EDITOR_RUNNING) {
        fputs(error_buf, stderr);
        fputc('\n', stderr);
    }
}

void info_msg(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsnprintf(error_buf, sizeof(error_buf), format, ap);
    va_end(ap);
    msg_is_error = false;
}
