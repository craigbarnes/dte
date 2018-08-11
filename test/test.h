#ifndef TEST_H
#define TEST_H

#include <inttypes.h>
#include <stddef.h>
#include "../src/util/macros.h"
#include "../src/common.h"

#define FOR_EACH_I(i, array) \
    for (size_t i = 0; i < ARRAY_COUNT(array); i++)

#define EXPECT_EQ(a, b) do { \
    if ((a) != (b)) { \
        fail ( \
            "%s:%d: Values not equal: %" PRIdMAX ", %" PRIdMAX "\n", \
            __FILE__, \
            __LINE__, \
            (intmax_t)(a), \
            (intmax_t)(b) \
        ); \
    } \
} while (0)

#define IEXPECT_EQ(a, b, i, s) do { \
    if ((a) != (b)) { \
        fail ( \
            "%s:%d: Test #%zu: %s not equal: %" PRIdMAX ", %" PRIdMAX "\n", \
            __FILE__, \
            __LINE__, \
            ((i) + 1), \
            (s), \
            (intmax_t)(a), \
            (intmax_t)(b) \
        ); \
    } \
} while (0)

#define EXPECT_STREQ(a, b) do { \
    const char *s1 = (a), *s2 = (b); \
    if (unlikely(!xstreq(s1, s2))) { \
        fail ( \
            "%s:%d: Strings not equal: '%s', '%s'\n", \
            __FILE__, \
            __LINE__, \
            s1 ? s1 : "(null)", \
            s2 ? s2 : "(null)" \
        ); \
    } \
} while (0)

extern unsigned int failed;

void fail(const char *format, ...) PRINTF(1);

#if GNUC_AT_LEAST(4, 2)
# pragma GCC diagnostic ignored "-Wmissing-prototypes"
#elif defined(__clang__) && HAS_WARNING("-Wmissing-prototypes")
# pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif

#endif
