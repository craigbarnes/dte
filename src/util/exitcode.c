#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include "exitcode.h"
#include "xreadwrite.h"

ExitCode ec_error(const char *prefix, ExitCode ec)
{
    perror(prefix);
    return ec;
}

ExitCode ec_write_stdout(const char *str, size_t len)
{
    ssize_t r = xwrite_all(STDOUT_FILENO, str, len);
    return likely(r >= 0) ? EC_OK : ec_error("write", EC_IO_ERROR);
}

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
