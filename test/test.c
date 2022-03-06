#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "test.h"
#include "util/str-util.h"
#include "util/utf8.h"

unsigned int passed, failed;

#define expect(cond, ...) if (cond) passed++; else test_fail(__VA_ARGS__)

static void abort_if_false(bool cond)
{
    if (unlikely(!cond)) {
        abort();
    }
}

static bool make_printable_str(const char **s1, const char **s2, bool cond)
{
    if (unlikely(!cond)) {
        *s1 = *s1 ? *s1 : "(null)";
        *s2 = *s2 ? *s2 : "(null)";
    }
    return cond;
}

size_t make_printable_mem(const char *src, size_t src_len, char *dest, size_t destsize)
{
    BUG_ON(destsize < 16);
    size_t len = 0;
    for (size_t i = 0; i < src_len && len < destsize - 5; ) {
        u_set_char(dest, &len, u_get_char(src, src_len, &i));
    }
    dest[len] = '\0';
    return len;
}

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
    bool cond = make_printable_str(&s1, &s2, xstreq(s1, s2));
    expect(cond, file, line, "Strings not equal: '%s', '%s'", s1, s2);
}

void expect_ptreq(const char *file, int line, const void *p1, const void *p2)
{
    expect(p1 == p2, file, line, "Pointers not equal: %p, %p", p1, p2);
}

void expect_memeq(const char *file, int line, const void *m1, const void *m2, size_t len)
{
    if (mem_equal(m1, m2, len)) {
        passed++;
        return;
    }
    char buf1[256], buf2[256];
    make_printable_mem(m1, len, buf1, sizeof buf1);
    make_printable_mem(m2, len, buf2, sizeof buf2);
    test_fail(file, line, "Bytes not equal:  %s  %s", buf1, buf2);
}

void expect_eq(const char *file, int line, intmax_t a, intmax_t b)
{
    expect(a == b, file, line, "Values not equal: %jd, %jd", a, b);
}

void expect_uint_eq(const char *file, int line, uintmax_t a, uintmax_t b)
{
    expect(a == b, file, line, "Values not equal: 0x%jx, 0x%jx", a, b);
}

void expect_true(const char *file, int line, bool x)
{
    expect(x, file, line, "Unexpected false value");
}

void expect_false(const char *file, int line, bool x)
{
    expect(!x, file, line, "Unexpected true value");
}

void expect_null(const char *file, int line, const void *ptr)
{
    expect(ptr == NULL, file, line, "Expected NULL, but got: %p", ptr);
}

void expect_nonnull(const char *file, int line, const void *ptr)
{
    expect(ptr != NULL, file, line, "Unexpected NULL pointer");
}

void iexpect_streq(const char *f, int l, size_t i, const char *a, const char *b)
{
    bool cond = make_printable_str(&a, &b, xstreq(a, b));
    expect(cond, f, l, "Test #%zu: strings not equal: '%s', '%s'", ++i, a, b);
}

void iexpect_eq(const char *file, int line, size_t i, intmax_t a, intmax_t b)
{
    expect(a == b, file, line, "Test #%zu: values not equal: %jd, %jd", ++i, a, b);
}

void iexpect_true(const char *file, int line, size_t i, bool x)
{
    expect(x, file, line, "Test #%zu: unexpected false value", ++i);
}

void assert_ptreq(const char *file, int line, const void *p1, const void *p2)
{
    expect_ptreq(file, line, p1, p2);
    abort_if_false(p1 == p2);
}

void assert_eq(const char *file, int line, intmax_t a, intmax_t b)
{
    expect_eq(file, line, a, b);
    abort_if_false(a == b);
}

void assert_true(const char *file, int line, bool x)
{
    expect_true(file, line, x);
    abort_if_false(x);
}

void assert_nonnull(const char *file, int line, const void *ptr)
{
    expect_nonnull(file, line, ptr);
    abort_if_false(ptr != NULL);
}
