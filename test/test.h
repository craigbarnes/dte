#ifndef TEST_TEST_H
#define TEST_TEST_H

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "util/macros.h"

typedef struct {
    unsigned int passed;
    unsigned int failed;
    void *userdata;
    char boldred[8];
    char dim[5];
    char sgr0[5];
    bool timing;
} TestContext;

typedef struct {
    const char *name;
    void (*func)(TestContext *ctx);
} TestEntry;

typedef struct {
    const TestEntry *tests;
    size_t nr_tests;
} TestGroup;

#define TEST(e) { \
    .name = #e, \
    .func = e \
}

#define TEST_GROUP(t) { \
    .tests = t, \
    .nr_tests = ARRAYLEN(t) \
}

#define FOR_EACH_I(i, array) \
    for (size_t i = 0; i < ARRAYLEN(array); i++)

#define TEST_FAIL(...) test_fail(ctx, __FILE__, __LINE__, __VA_ARGS__)
#define EXPECT(fn, ...) expect_##fn(ctx, __FILE__, __LINE__, __VA_ARGS__)
#define IEXPECT(fn, ...) iexpect_##fn(ctx, __FILE__, __LINE__, i, __VA_ARGS__)
#define ASSERT(fn, ...) assert_##fn(ctx, __FILE__, __LINE__, __VA_ARGS__)

#define EXPECT_STREQ(s1, s2) EXPECT(streq, s1, s2)
#define EXPECT_PTREQ(p1, p2) EXPECT(ptreq, p1, p2)
#define EXPECT_MEMEQ(m1, m2, len) EXPECT(memeq, m1, m2, len)
#define EXPECT_EQ(a, b) EXPECT(eq, a, b)
#define EXPECT_UINT_EQ(a, b) EXPECT(uint_eq, a, b)
#define EXPECT_NULL(p) EXPECT(null, p)
#define EXPECT_NONNULL(p) EXPECT(nonnull, p)
#define EXPECT_TRUE(x) EXPECT(true, x)
#define EXPECT_FALSE(x) EXPECT(false, x)
#define IEXPECT_EQ(a, b) IEXPECT(eq, a, b)
#define IEXPECT_STREQ(s1, s2) IEXPECT(streq, s1, s2)
#define IEXPECT_TRUE(x) IEXPECT(true, x)
#define ASSERT_PTREQ(p1, p2) ASSERT(ptreq, p1, p2)
#define ASSERT_EQ(a, b) ASSERT(eq, a, b)
#define ASSERT_TRUE(x) ASSERT(true, x)
#define ASSERT_NONNULL(ptr) ASSERT(nonnull, ptr)

void test_fail(TestContext *ctx, const char *file, int line, const char *format, ...) COLD PRINTF(4);
void expect_streq(TestContext *ctx, const char *file, int line, const char *s1, const char *s2);
void expect_ptreq(TestContext *ctx, const char *file, int line, const void *p1, const void *p2);
void expect_memeq(TestContext *ctx, const char *file, int line, const void *m1, const void *m2, size_t len);
void expect_eq(TestContext *ctx, const char *file, int line, intmax_t a, intmax_t b);
void expect_uint_eq(TestContext *ctx, const char *file, int line, uintmax_t a, uintmax_t b);
void expect_true(TestContext *ctx, const char *file, int line, bool x);
void expect_false(TestContext *ctx, const char *file, int line, bool x);
void expect_null(TestContext *ctx, const char *file, int line, const void *p);
void expect_nonnull(TestContext *ctx, const char *file, int line, const void *p);
void iexpect_streq(TestContext *ctx, const char *file, int line, size_t i, const char *s1, const char *s2);
void iexpect_eq(TestContext *ctx, const char *file, int line, size_t i, intmax_t a, intmax_t b);
void iexpect_true(TestContext *ctx, const char *file, int line, size_t i, bool x);
void assert_ptreq(TestContext *ctx, const char *file, int line, const void *p1, const void *p2);
void assert_eq(TestContext *ctx, const char *file, int line, intmax_t a, intmax_t b);
void assert_true(TestContext *ctx, const char *file, int line, bool x);
void assert_nonnull(TestContext *ctx, const char *file, int line, const void *ptr);

#endif
