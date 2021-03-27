#ifndef TEST_TEST_H
#define TEST_TEST_H

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include "util/macros.h"

extern unsigned int passed, failed;

typedef struct {
    const char *name;
    void (*func)(void);
} TestEntry;

typedef struct {
    const TestEntry *tests;
    size_t nr_tests;
} TestGroup;

#define TEST(e) (TestEntry) { \
    .name = #e, \
    .func = e \
}

#define TEST_GROUP(t) { \
    .tests = t, \
    .nr_tests = ARRAY_COUNT(t) \
}

#define FOR_EACH_I(i, array) \
    for (size_t i = 0; i < ARRAY_COUNT(array); i++)

#define TEST_FAIL(...) test_fail(__FILE__, __LINE__, __VA_ARGS__)
#define EXPECT(fn, ...) expect_##fn(__FILE__, __LINE__, __VA_ARGS__)
#define IEXPECT(fn, ...) iexpect_##fn(__FILE__, __LINE__, i, __VA_ARGS__)
#define ASSERT(fn, ...) assert_##fn(__FILE__, __LINE__, __VA_ARGS__)

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

void test_fail(const char *file, int line, const char *format, ...) PRINTF(3);
void expect_streq(const char *file, int line, const char *s1, const char *s2);
void expect_ptreq(const char *file, int line, const void *p1, const void *p2);
void expect_memeq(const char *file, int line, const void *m1, const void *m2, size_t len);
void expect_eq(const char *file, int line, intmax_t a, intmax_t b);
void expect_uint_eq(const char *file, int line, uintmax_t a, uintmax_t b);
void expect_true(const char *file, int line, bool x);
void expect_false(const char *file, int line, bool x);
void expect_null(const char *file, int line, const void *p);
void expect_nonnull(const char *file, int line, const void *p);
void iexpect_streq(const char *file, int line, size_t i, const char *s1, const char *s2);
void iexpect_eq(const char *file, int line, size_t i, intmax_t a, intmax_t b);
void iexpect_true(const char *file, int line, size_t i, bool x);
void assert_ptreq(const char *file, int line, const void *p1, const void *p2);
void assert_eq(const char *file, int line, intmax_t a, intmax_t b);
void assert_true(const char *file, int line, bool x);
void assert_nonnull(const char *file, int line, const void *ptr);

#endif
