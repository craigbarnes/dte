#ifndef TEST_TEST_H
#define TEST_TEST_H

#include <inttypes.h>
#include <stddef.h>
#include "../src/util/macros.h"
#include "../src/common.h"

#define FOR_EACH_I(i, array) \
    for (size_t i = 0; i < ARRAY_COUNT(array); i++)

#define FAIL(fmt, ...) fail(__FILE__, __LINE__, fmt, __VA_ARGS__)

#define EXPECT_STREQ(s1, s2) expect_streq(__FILE__, __LINE__, s1, s2)
#define EXPECT_EQ(a, b) expect_eq(__FILE__, __LINE__, a, b)
#define EXPECT_TRUE(x) EXPECT_EQ(!!(x), 1)
#define EXPECT_FALSE(x) EXPECT_EQ(x, 0)

#define IEXPECT_EQ(line, a, b) expect_eq(__FILE__, line, a, b)
#define IEXPECT_STREQ(line, s1, s2) expect_streq(__FILE__, line, s1, s2)

extern unsigned int failed;

void fail(const char *file, int line, const char *format, ...) PRINTF(3);
void expect_streq(const char *file, int line, const char *s1, const char *s2);
void expect_eq(const char *file, int line, intmax_t a, intmax_t b);

#if GNUC_AT_LEAST(4, 2)
# pragma GCC diagnostic ignored "-Wmissing-prototypes"
#elif defined(__clang__) && HAS_WARNING("-Wmissing-prototypes")
# pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif

#endif
