#include <stdarg.h>
#include "exitcode.h"

ExitCode ec_usage_error(const char *restrict fmt, ...)
{
    (void)!fputs("Error: ", stderr);
    va_list v;
    va_start(v, fmt);
    (void)!vfprintf(stderr, fmt, v);
    va_end(v);
    (void)!fputc('\n', stderr);
    return EC_USAGE_ERROR;
}

ExitCode ec_printf_ok(const char *restrict fmt, ...)
{
    va_list v;
    va_start(v, fmt);
    (void)!vfprintf(stdout, fmt, v);
    va_end(v);
    return EC_OK;
}
