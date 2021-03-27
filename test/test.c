#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "test.h"
#include "util/str-util.h"

unsigned int passed, failed;

void test_fail(const char *file, int line, const char *format, ...)
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
        s1 = s1 ? s1 : "(null)";
        s2 = s2 ? s2 : "(null)";
        test_fail(file, line, "Strings not equal: '%s', '%s'", s1, s2);
    } else {
        passed++;
    }
}

void expect_ptreq(const char *file, int line, const void *p1, const void *p2)
{
    if (unlikely(p1 != p2)) {
        test_fail(file, line, "Pointers not equal: %p, %p", p1, p2);
    } else {
        passed++;
    }
}

void expect_memeq(const char *file, int line, const void *m1, const void *m2, size_t len)
{
    if (unlikely(!mem_equal(m1, m2, len))) {
        test_fail(file, line, "Bytes not equal");
    } else {
        passed++;
    }
}

void expect_eq(const char *file, int line, intmax_t a, intmax_t b)
{
    if (unlikely(a != b)) {
        test_fail(file, line, "Values not equal: %jd, %jd", a, b);
    } else {
        passed++;
    }
}

void expect_uint_eq(const char *file, int line, uintmax_t a, uintmax_t b)
{
    if (unlikely(a != b)) {
        test_fail(file, line, "Values not equal: 0x%jx, 0x%jx", a, b);
    } else {
        passed++;
    }
}

void expect_true(const char *file, int line, bool x)
{
    if (unlikely(!x)) {
        test_fail(file, line, "Unexpected false value");
    } else {
        passed++;
    }
}

void expect_false(const char *file, int line, bool x)
{
    if (unlikely(x)) {
        test_fail(file, line, "Unexpected true value");
    } else {
        passed++;
    }
}

void expect_null(const char *file, int line, const void *ptr)
{
    if (unlikely(ptr != NULL)) {
        test_fail(file, line, "Expected NULL, but got: %p", ptr);
    } else {
        passed++;
    }
}

void expect_nonnull(const char *file, int line, const void *ptr)
{
    if (unlikely(ptr == NULL)) {
        test_fail(file, line, "Unexpected NULL pointer");
    } else {
        passed++;
    }
}

void iexpect_streq(const char *f, int l, size_t i, const char *a, const char *b)
{
    if (unlikely(!xstreq(a, b))) {
        a = a ? a : "(null)";
        b = b ? b : "(null)";
        i++;
        test_fail(f, l, "Test #%zu: strings not equal: '%s', '%s'", i, a, b);
    } else {
        passed++;
    }
}

void iexpect_eq(const char *file, int line, size_t i, intmax_t a, intmax_t b)
{
    if (unlikely(a != b)) {
        i++;
        test_fail(file, line, "Test #%zu: values not equal: %jd, %jd", i, a, b);
    } else {
        passed++;
    }
}

void iexpect_true(const char *file, int line, size_t i, bool x)
{
    if (unlikely(!x)) {
        i++;
        test_fail(file, line, "Test #%zu: unexpected false value", i);
    } else {
        passed++;
    }
}

void assert_ptreq(const char *file, int line, const void *p1, const void *p2)
{
    if (unlikely(p1 != p2)) {
        test_fail(file, line, "ERROR: Pointers not equal: %p, %p", p1, p2);
        abort();
    } else {
        passed++;
    }
}

void assert_eq(const char *file, int line, intmax_t a, intmax_t b)
{
    if (unlikely(a != b)) {
        test_fail(file, line, "ERROR: Values not equal: %jd, %jd", a, b);
        abort();
    } else {
        passed++;
    }
}

void assert_true(const char *file, int line, bool x)
{
    if (unlikely(!x)) {
        test_fail(file, line, "ERROR: Unexpected false value");
        abort();
    } else {
        passed++;
    }
}

void assert_nonnull(const char *file, int line, const void *ptr)
{
    if (unlikely(ptr == NULL)) {
        test_fail(file, line, "ERROR: Unexpected NULL pointer");
        abort();
    } else {
        passed++;
    }
}
