#ifndef TEST_TEST_H
#define TEST_TEST_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include "../src/util/macros.h"

#define FOR_EACH_I(i, array) \
    for (size_t i = 0; i < ARRAY_COUNT(array); i++)

#define TEST_FAIL(...) test_fail(__FILE__, __LINE__, __VA_ARGS__)

#define EXPECT_STREQ(s1, s2) expect_streq(__FILE__, __LINE__, s1, s2)
#define EXPECT_PTREQ(p1, p2) expect_ptreq(__FILE__, __LINE__, p1, p2)
#define EXPECT_EQ(a, b) expect_eq(__FILE__, __LINE__, a, b)
#define EXPECT_UINT_EQ(a, b) expect_uint_eq(__FILE__, __LINE__, a, b)
#define EXPECT_NULL(p) expect_null(__FILE__, __LINE__, p)
#define EXPECT_NONNULL(p) expect_nonnull(__FILE__, __LINE__, p)
#define EXPECT_TRUE(x) expect_true(__FILE__, __LINE__, x)
#define EXPECT_FALSE(x) expect_false(__FILE__, __LINE__, x)

#define IEXPECT_EQ(a, b) iexpect_eq(__FILE__, __LINE__, i, a, b)
#define IEXPECT_STREQ(s1, s2) iexpect_streq(__FILE__, __LINE__, i, s1, s2)
#define IEXPECT_TRUE(x) iexpect_true(__FILE__, __LINE__, i, x)

#define ASSERT_EQ(a, b) assert_eq(__FILE__, __LINE__, a, b)
#define ASSERT_TRUE(x) assert_true(__FILE__, __LINE__, x)
#define ASSERT_NONNULL(ptr) assert_nonnull(__FILE__, __LINE__, ptr)

extern unsigned int failed;

void test_fail(const char *file, int line, const char *format, ...) PRINTF(3);

void expect_streq(const char *file, int line, const char *s1, const char *s2);
void expect_ptreq(const char *file, int line, const void *p1, const void *p2);
void expect_eq(const char *file, int line, intmax_t a, intmax_t b);
void expect_uint_eq(const char *file, int line, uintmax_t a, uintmax_t b);
void expect_true(const char *file, int line, bool x);
void expect_false(const char *file, int line, bool x);
void expect_null(const char *file, int line, const void *p);
void expect_nonnull(const char *file, int line, const void *p);

void iexpect_streq(const char *file, int line, size_t i, const char *s1, const char *s2);
void iexpect_eq(const char *file, int line, size_t i, intmax_t a, intmax_t b);
void iexpect_true(const char *file, int line, size_t i, bool x);

void assert_eq(const char *file, int line, intmax_t a, intmax_t b);
void assert_true(const char *file, int line, bool x);
void assert_nonnull(const char *file, int line, const void *ptr);

#endif
