#include "test.h"
#include "bookmark.h"
#include "util/ptr-array.h"

static void test_bookmark_push(TestContext *ctx)
{
    PointerArray bookmarks;
    ptr_array_init(&bookmarks, 80);
    ASSERT_NONNULL(bookmarks.ptrs);
    ASSERT_TRUE(bookmarks.alloc >= 80);
    ASSERT_EQ(bookmarks.count, 0);

    size_t n = 300;
    for (size_t i = 0; i < n; i++) {
        FileLocation *loc = xmalloc(sizeof(*loc));
        *loc = (FileLocation){.buffer_id = i};
        bookmark_push(&bookmarks, loc);
    }

    ASSERT_EQ(bookmarks.count, 256);
    ASSERT_TRUE(bookmarks.alloc >= 256);

    const FileLocation *loc = bookmarks.ptrs[0];
    EXPECT_EQ(loc->buffer_id, n - 256);
    loc = bookmarks.ptrs[255];
    EXPECT_EQ(loc->buffer_id, n - 1);

    ptr_array_free_cb(&bookmarks, FREE_FUNC(file_location_free));
}

static const TestEntry tests[] = {
    TEST(test_bookmark_push),
};

const TestGroup bookmark_tests = TEST_GROUP(tests);
