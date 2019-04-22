#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "test.h"
#include "../src/common.h"

unsigned int failed;

PRINTF(3)
static void test_fail(const char *file, int line, const char *format, ...)
{
    fprintf(stderr, "%s:%d: ", file, line);
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    putc('\n', stderr);
    fflush(stderr);
    failed += 1;
}

void expect_streq(const char *file, int line, const char *s1, const char *s2)
{
    if (unlikely(!xstreq(s1, s2))) {
        test_fail (
            file,
            line,
            "Strings not equal: '%s', '%s'",
            s1 ? s1 : "(null)",
            s2 ? s2 : "(null)"
        );
    }
}

void expect_ptreq(const char *file, int line, const void *p1, const void *p2)
{
    if (unlikely(p1 != p2)) {
        test_fail(file, line, "Pointers not equal: %p, %p", p1, p2);
    }
}

void expect_eq(const char *file, int line, intmax_t a, intmax_t b)
{
    if (unlikely(a != b)) {
        test_fail (
            file,
            line,
            "Values not equal: %" PRIdMAX ", %" PRIdMAX,
            a,
            b
        );
    }
}

void iexpect_streq(const char *file, int line, size_t i, const char *s1, const char *s2)
{
    if (unlikely(!xstreq(s1, s2))) {
        test_fail (
            file,
            line,
            "Test #%zu: strings not equal: '%s', '%s'",
            i + 1,
            s1 ? s1 : "(null)",
            s2 ? s2 : "(null)"
        );
    }
}

void iexpect_eq(const char *file, int line, size_t i, intmax_t a, intmax_t b)
{
    if (unlikely(a != b)) {
        test_fail (
            file,
            line,
            "Test #%zu: values not equal: %" PRIdMAX ", %" PRIdMAX,
            i + 1,
            a,
            b
        );
    }
}

void assert_eq(const char *file, int line, intmax_t a, intmax_t b)
{
    if (unlikely(a != b)) {
        test_fail (
            file,
            line,
            "ERROR: Values not equal: %" PRIdMAX ", %" PRIdMAX,
            a,
            b
        );
        abort();
    }
}
