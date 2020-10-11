#include "defs.h"
#include <stdio.h>

char *ftest_vasprintf(const char *fmt, va_list ap);

char *ftest_vasprintf(const char *fmt, va_list ap)
{
    char *str;
    int n = (vasprintf)(&str, fmt, ap);
    return (n >= 0) ? str : NULL;
}

int main(void)
{
    return 0;
}
