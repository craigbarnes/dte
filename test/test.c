#include <stdarg.h>
#include <stdio.h>
#include "test.h"

unsigned int failed;

void fail(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    failed += 1;
}
