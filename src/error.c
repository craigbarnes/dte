#include <stdio.h>
#include "error.h"
#include "config.h"
#include "debug.h"
#include "editor.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

static char error_buf[256];
const char *const error_ptr = error_buf;
unsigned int nr_errors;
bool msg_is_error;
bool supress_error_msg;

Error *error_create_errno(int code, const char *format, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, format);
    int n = vsnprintf(buf, sizeof buf, format, ap);
    va_end(ap);
    BUG_ON(n < 0);

    Error *err = xmalloc(sizeof(Error) + n + 1);
    memcpy(err->msg, buf, n + 1);
    err->code = code;
    return err;
}

void error_free(Error *err)
{
    free(err);
}

void clear_error(void)
{
    error_buf[0] = '\0';
}

void error_msg(const char *format, ...)
{
    if (supress_error_msg) {
        return;
    }

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
