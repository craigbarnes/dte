#include <string.h>
#include "test.h"
#include "util/bitset.h"

static void test_bitset(void)
{
    BitSetWord set[BITSET_NR_WORDS(256)];
    ASSERT_TRUE(sizeof(set) >= 32);

    memset(set, 0, sizeof(set));
    EXPECT_FALSE(bitset_contains(set, '0'));
    EXPECT_FALSE(bitset_contains(set, 'a'));
    EXPECT_FALSE(bitset_contains(set, 'z'));
    EXPECT_FALSE(bitset_contains(set, '!'));
    EXPECT_FALSE(bitset_contains(set, '\0'));

    bitset_add_char_range(set, "0-9a-fxy");
    EXPECT_TRUE(bitset_contains(set, '0'));
    EXPECT_TRUE(bitset_contains(set, '8'));
    EXPECT_TRUE(bitset_contains(set, '9'));
    EXPECT_TRUE(bitset_contains(set, 'a'));
    EXPECT_TRUE(bitset_contains(set, 'b'));
    EXPECT_TRUE(bitset_contains(set, 'f'));
    EXPECT_TRUE(bitset_contains(set, 'x'));
    EXPECT_TRUE(bitset_contains(set, 'y'));
    EXPECT_FALSE(bitset_contains(set, 'g'));
    EXPECT_FALSE(bitset_contains(set, 'z'));
    EXPECT_FALSE(bitset_contains(set, 'A'));
    EXPECT_FALSE(bitset_contains(set, 'F'));
    EXPECT_FALSE(bitset_contains(set, 'X'));
    EXPECT_FALSE(bitset_contains(set, 'Z'));
    EXPECT_FALSE(bitset_contains(set, '{'));
    EXPECT_FALSE(bitset_contains(set, '`'));
    EXPECT_FALSE(bitset_contains(set, '/'));
    EXPECT_FALSE(bitset_contains(set, ':'));
    EXPECT_FALSE(bitset_contains(set, '\0'));

    BITSET_INVERT(set);
    EXPECT_FALSE(bitset_contains(set, '0'));
    EXPECT_FALSE(bitset_contains(set, '8'));
    EXPECT_FALSE(bitset_contains(set, '9'));
    EXPECT_FALSE(bitset_contains(set, 'a'));
    EXPECT_FALSE(bitset_contains(set, 'b'));
    EXPECT_FALSE(bitset_contains(set, 'f'));
    EXPECT_FALSE(bitset_contains(set, 'x'));
    EXPECT_FALSE(bitset_contains(set, 'y'));
    EXPECT_TRUE(bitset_contains(set, 'g'));
    EXPECT_TRUE(bitset_contains(set, 'z'));
    EXPECT_TRUE(bitset_contains(set, 'A'));
    EXPECT_TRUE(bitset_contains(set, 'F'));
    EXPECT_TRUE(bitset_contains(set, 'X'));
    EXPECT_TRUE(bitset_contains(set, 'Z'));
    EXPECT_TRUE(bitset_contains(set, '{'));
    EXPECT_TRUE(bitset_contains(set, '`'));
    EXPECT_TRUE(bitset_contains(set, '/'));
    EXPECT_TRUE(bitset_contains(set, ':'));
    EXPECT_TRUE(bitset_contains(set, '\0'));

    memset(set, 0, sizeof(set));
    FOR_EACH_I(i, set) {
        EXPECT_UINT_EQ(set[i], 0);
    }
    BITSET_INVERT(set);
    FOR_EACH_I(i, set) {
        EXPECT_UINT_EQ(set[i], ((BitSetWord)-1));
    }
}

DISABLE_WARNING("-Wmissing-prototypes")

void test_syntax(void)
{
    test_bitset();
}
