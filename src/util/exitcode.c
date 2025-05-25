#include <stdarg.h>
#include "exitcode.h"

ExitCode ec_usage_error(const char *restrict fmt, ...)
{
    fputs("Error: ", stderr);
    va_list v;
    va_start(v, fmt);
    vfprintf(stderr, fmt, v);
    va_end(v);
    fputc('\n', stderr);
    return EC_USAGE_ERROR;
}

ExitCode ec_printf_ok(const char *restrict fmt, ...)
{
    va_list v;
    va_start(v, fmt);
    vfprintf(stdout, fmt, v);
    va_end(v);
    return EC_OK;
}
