#include <stdarg.h>
#include "exitcode.h"

VPRINTF(1) NONNULL_ARGS
static void ec_verrorf(const char *restrict fmt, va_list v)
{
    (void)!fputs("Error: ", stderr);
    (void)!vfprintf(stderr, fmt, v);
    (void)!fputc('\n', stderr);
}

ExitCode ec_usage_error(const char *restrict fmt, ...)
{
    va_list v;
    va_start(v, fmt);
    ec_verrorf(fmt, v);
    va_end(v);
    return EC_USAGE_ERROR;
}

ExitCode ec_io_error(const char *restrict fmt, ...)
{
    va_list v;
    va_start(v, fmt);
    ec_verrorf(fmt, v);
    va_end(v);
    return EC_IO_ERROR;
}

ExitCode ec_os_error(const char *restrict fmt, ...)
{
    va_list v;
    va_start(v, fmt);
    ec_verrorf(fmt, v);
    va_end(v);
    return EC_OS_ERROR;
}

ExitCode ec_printf_ok(const char *restrict fmt, ...)
{
    va_list v;
    va_start(v, fmt);
    (void)!vfprintf(stdout, fmt, v);
    va_end(v);
    return EC_OK;
}
