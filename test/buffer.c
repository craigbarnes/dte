#include <stdlib.h>
#include "test.h"
#include "buffer.h"
#include "util/path.h"

static void test_find_buffer_by_id(void)
{
    ASSERT_NONNULL(buffer);
    EXPECT_PTREQ(find_buffer_by_id(buffer->id), buffer);

    const unsigned long large_id = 1UL << 30;
    static_assert_compatible_types(large_id, buffer->id);
    EXPECT_NULL(find_buffer_by_id(large_id));
}

static void test_short_filename(void)
{
    const char *rel = "test/main.c";
    char *abs = path_absolute(rel);
    char *s = short_filename(abs);
    EXPECT_STREQ(s, rel);
    free(abs);
    free(s);

    rel = "build/test/HOME/example";
    abs = path_absolute(rel);
    s = short_filename(abs);
    EXPECT_STREQ(s, "~/example");
    free(abs);
    free(s);
}

static const TestEntry tests[] = {
    TEST(test_find_buffer_by_id),
    TEST(test_short_filename),
};

const TestGroup buffer_tests = TEST_GROUP(tests);
