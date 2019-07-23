#include <stdio.h>
#include "error.h"
#include "config.h"
#include "editor.h"

static char error_buf[256];
const char *const error_ptr = error_buf;
unsigned int nr_errors;
bool msg_is_error;
bool supress_error_msg;

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
