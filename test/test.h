#ifndef TEST_TEST_H
#define TEST_TEST_H

#include <inttypes.h>
#include <stddef.h>
#include "../src/util/macros.h"

#define FOR_EACH_I(i, array) \
    for (size_t i = 0; i < ARRAY_COUNT(array); i++)

#define EXPECT_STREQ(s1, s2) expect_streq(__FILE__, __LINE__, s1, s2)
#define EXPECT_PTREQ(p1, p2) expect_ptreq(__FILE__, __LINE__, p1, p2)
#define EXPECT_EQ(a, b) expect_eq(__FILE__, __LINE__, a, b)
#define EXPECT_TRUE(x) EXPECT_EQ(!!(x), 1)
#define EXPECT_FALSE(x) EXPECT_EQ(x, 0)

#define IEXPECT_EQ(a, b) iexpect_eq(__FILE__, __LINE__, i, a, b)
#define IEXPECT_STREQ(s1, s2) iexpect_streq(__FILE__, __LINE__, i, s1, s2)
#define IEXPECT_TRUE(x) IEXPECT_EQ(!!(x), 1)
#define IEXPECT_FALSE(x) IEXPECT_EQ(x, 0)
#define IEXPECT_GT(a, b) IEXPECT_TRUE(a > b)

#define ASSERT_EQ(a, b) assert_eq(__FILE__, __LINE__, a, b)
#define ASSERT_TRUE(x) ASSERT_EQ(!!(x), 1)

extern unsigned int failed;

void expect_streq(const char *file, int line, const char *s1, const char *s2);
void expect_ptreq(const char *file, int line, const void *p1, const void *p2);
void expect_eq(const char *file, int line, intmax_t a, intmax_t b);
void iexpect_streq(const char *file, int line, size_t i, const char *s1, const char *s2);
void iexpect_eq(const char *file, int line, size_t i, intmax_t a, intmax_t b);

void assert_eq(const char *file, int line, intmax_t a, intmax_t b);

#if GNUC_AT_LEAST(4, 2)
# pragma GCC diagnostic ignored "-Wmissing-prototypes"
#elif defined(__clang__) && HAS_WARNING("-Wmissing-prototypes")
# pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif

#endif
