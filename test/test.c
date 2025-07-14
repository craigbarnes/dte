#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "test.h"
#include "util/utf8.h"

#define expect(cond, ctx, ...) if (cond) test_pass(ctx); else test_fail(ctx, __VA_ARGS__)

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

void test_fail(TestContext *ctx, const char *file, int line, const char *format, ...)
{
    fprintf(stderr, "%s:%d: ", file, line);
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    putc('\n', stderr);
    fflush(stderr);
    ctx->failed += 1;
}

void expect_streq(TestContext *ctx, const char *file, int line, const char *s1, const char *s2)
{
    bool cond = make_printable_str(&s1, &s2, xstreq(s1, s2));
    expect(cond, ctx, file, line, "Strings not equal: '%s', '%s'", s1, s2);
}

void expect_ptreq(TestContext *ctx, const char *file, int line, const void *p1, const void *p2)
{
    expect(p1 == p2, ctx, file, line, "Pointers not equal: %p, %p", p1, p2);
}

void expect_memeq (
    TestContext *ctx,
    const char *file,
    int line,
    const void *p1,
    size_t n1,
    const void *p2,
    size_t n2
) {
    if (likely(n1 == n2 && mem_equal(p1, p2, n1))) {
        test_pass(ctx);
        return;
    }

    char buf[2048];
    size_t m1 = u_make_printable(p1, n1, buf, sizeof(buf) - 64, 0);
    size_t m2 = u_make_printable(p2, n2, buf + m1, sizeof(buf) - m1, 0);

    test_fail (
        ctx, file, line,
        "Memory areas not equal: lengths: %zu, %zu; contents:  %.*s  %.*s",
        n1, n2, (int)m1, buf, (int)m2, buf + m1
    );
}

void expect_eq(TestContext *ctx, const char *file, int line, intmax_t a, intmax_t b)
{
    expect(a == b, ctx, file, line, "Values not equal: %jd, %jd", a, b);
}

void expect_ne(TestContext *ctx, const char *file, int line, intmax_t a, intmax_t b)
{
    expect(a != b, ctx, file, line, "Unexpected equal values: %jd, %jd", a, b);
}

void expect_uint_eq(TestContext *ctx, const char *file, int line, uintmax_t a, uintmax_t b)
{
    expect(a == b, ctx, file, line, "Values not equal: 0x%jx, 0x%jx", a, b);
}

void expect_true(TestContext *ctx, const char *file, int line, bool x)
{
    expect(x, ctx, file, line, "Unexpected false value");
}

void expect_false(TestContext *ctx, const char *file, int line, bool x)
{
    expect(!x, ctx, file, line, "Unexpected true value");
}

void expect_null(TestContext *ctx, const char *file, int line, const void *ptr)
{
    expect(ptr == NULL, ctx, file, line, "Expected NULL, but got: %p", ptr);
}

void expect_nonnull(TestContext *ctx, const char *file, int line, const void *ptr)
{
    expect(ptr != NULL, ctx, file, line, "Unexpected NULL pointer");
}

void iexpect_streq(TestContext *ctx, const char *file, int line, size_t i, const char *s1, const char *s2)
{
    bool cond = make_printable_str(&s1, &s2, xstreq(s1, s2));
    expect(cond, ctx, file, line, "Test #%zu: strings not equal: '%s', '%s'", ++i, s1, s2);
}

void iexpect_eq(TestContext *ctx, const char *file, int line, size_t i, intmax_t a, intmax_t b)
{
    expect(a == b, ctx, file, line, "Test #%zu: values not equal: %jd, %jd", ++i, a, b);
}

void iexpect_true(TestContext *ctx, const char *file, int line, size_t i, bool x)
{
    expect(x, ctx, file, line, "Test #%zu: unexpected false value", ++i);
}

void assert_ptreq(TestContext *ctx, const char *file, int line, const void *p1, const void *p2)
{
    expect_ptreq(ctx, file, line, p1, p2);
    abort_if_false(p1 == p2);
}

void assert_eq(TestContext *ctx, const char *file, int line, intmax_t a, intmax_t b)
{
    expect_eq(ctx, file, line, a, b);
    abort_if_false(a == b);
}

void assert_ne(TestContext *ctx, const char *file, int line, intmax_t a, intmax_t b)
{
    expect_ne(ctx, file, line, a, b);
    abort_if_false(a != b);
}

void assert_true(TestContext *ctx, const char *file, int line, bool x)
{
    expect_true(ctx, file, line, x);
    abort_if_false(x);
}

void assert_nonnull(TestContext *ctx, const char *file, int line, const void *ptr)
{
    expect_nonnull(ctx, file, line, ptr);
    abort_if_false(ptr != NULL);
}
