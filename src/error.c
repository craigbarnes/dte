#include "error.h"
#include "editor.h"
#include "config.h"
#include "common.h"

int nr_errors;
bool msg_is_error;
char error_buf[256];

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
    err = error_new(xvsprintf(format, ap));
    va_end(ap);
    return err;
}

Error *error_create_errno(int code, const char *format, ...)
{
    Error *err;
    va_list ap;

    va_start(ap, format);
    err = error_new(xvsprintf(format, ap));
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
    va_list ap;
    size_t pos = 0;

    // Some implementations of *printf return -1 if output was truncated
    if (config_file) {
        snprintf (
            error_buf,
            sizeof(error_buf),
            "%s:%d: ",
            config_file,
            config_line
        );
        pos = strlen(error_buf);
        if (current_command) {
            snprintf (
                error_buf + pos,
                sizeof(error_buf) - pos,
                "%s: ",
                current_command->name
            );
            pos += strlen(error_buf + pos);
        }
    }

    va_start(ap, format);
    vsnprintf(error_buf + pos, sizeof(error_buf) - pos, format, ap);
    va_end(ap);

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
