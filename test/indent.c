#include "test.h"
#include "indent.h"

static void test_next_indent_width(TestContext *ctx)
{
    EXPECT_EQ(next_indent_width(0, 4), 4);
    EXPECT_EQ(next_indent_width(1, 4), 4);
    EXPECT_EQ(next_indent_width(2, 4), 4);
    EXPECT_EQ(next_indent_width(3, 4), 4);
    EXPECT_EQ(next_indent_width(4, 4), 8);
    EXPECT_EQ(next_indent_width(5, 4), 8);
    EXPECT_EQ(next_indent_width(6, 4), 8);
    EXPECT_EQ(next_indent_width(7, 4), 8);
    EXPECT_EQ(next_indent_width(8, 4), 12);
    EXPECT_EQ(next_indent_width(9, 4), 12);

    EXPECT_EQ(next_indent_width(0, 3), 3);
    EXPECT_EQ(next_indent_width(1, 3), 3);
    EXPECT_EQ(next_indent_width(2, 3), 3);
    EXPECT_EQ(next_indent_width(3, 3), 6);
    EXPECT_EQ(next_indent_width(4, 3), 6);
    EXPECT_EQ(next_indent_width(5, 3), 6);
    EXPECT_EQ(next_indent_width(6, 3), 9);
    EXPECT_EQ(next_indent_width(7, 3), 9);
    EXPECT_EQ(next_indent_width(8, 3), 9);
    EXPECT_EQ(next_indent_width(9, 3), 12);

    EXPECT_EQ(next_indent_width(0, 1), 1);
    EXPECT_EQ(next_indent_width(1, 1), 2);
    EXPECT_EQ(next_indent_width(2, 1), 3);
    EXPECT_EQ(next_indent_width(3, 1), 4);
    EXPECT_EQ(next_indent_width(4, 1), 5);

    for (size_t x = 0; x <= 17; x++) {
        for (size_t m = 1; m <= 8; m++) {
            size_t r = next_indent_width(x, m);
            EXPECT_TRUE(r > x);
            EXPECT_EQ(r % m, 0);
            EXPECT_EQ(r, ((x + m) / m) * m);
            if (x % m == 0) {
                EXPECT_EQ(r, x + m);
            }
        }
    }

    EXPECT_UINT_EQ(next_indent_width(SIZE_MAX - 0, 4), 0);
    EXPECT_UINT_EQ(next_indent_width(SIZE_MAX - 1, 4), 0);
    EXPECT_UINT_EQ(next_indent_width(SIZE_MAX - 2, 4), 0);
    EXPECT_UINT_EQ(next_indent_width(SIZE_MAX - 3, 4), 0);

    EXPECT_UINT_EQ(next_indent_width(SIZE_MAX - 0, 3), 0);
    EXPECT_UINT_EQ(next_indent_width(SIZE_MAX - 1, 3), 0);
    EXPECT_UINT_EQ(next_indent_width(SIZE_MAX - 2, 3), 0);

    EXPECT_UINT_EQ(next_indent_width(SIZE_MAX - 0, 1), 0);
}

static const TestEntry tests[] = {
    TEST(test_next_indent_width),
};

const TestGroup indent_tests = TEST_GROUP(tests);
