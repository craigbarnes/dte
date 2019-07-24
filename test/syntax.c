#include <string.h>
#include "test.h"
#include "../src/syntax/bitset.h"

static void test_bitset_invert(void)
{
    BitSet set = {0};
    ASSERT_EQ(sizeof(set), 32);
    ASSERT_EQ(ARRAY_COUNT(set), 32);

    memset(set, 0, sizeof(set));
    for (size_t i = 0; i < ARRAY_COUNT(set); i++) {
        EXPECT_UINT_EQ(set[i], 0);
    }

    bitset_invert(set);
    for (size_t i = 0; i < ARRAY_COUNT(set); i++) {
        EXPECT_UINT_EQ(set[i], UINT8_MAX);
    }
}

DISABLE_WARNING("-Wmissing-prototypes")

void test_syntax(void)
{
    test_bitset_invert();
}
