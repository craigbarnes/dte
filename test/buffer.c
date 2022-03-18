#include <stdlib.h>
#include "test.h"
#include "buffer.h"
#include "editor.h"
#include "indent.h"

static void test_find_buffer_by_id(TestContext *ctx)
{
    const Buffer *buffer = editor.buffer;
    ASSERT_NONNULL(buffer);
    EXPECT_PTREQ(find_buffer_by_id(&editor.buffers, buffer->id), buffer);

    const unsigned long large_id = 1UL << 30;
    static_assert_compatible_types(large_id, buffer->id);
    EXPECT_NULL(find_buffer_by_id(&editor.buffers, large_id));
}

static void test_make_indent(TestContext *ctx)
{
    Buffer *buffer = editor.buffer;
    ASSERT_NONNULL(buffer);
    const LocalOptions saved_opts = buffer->options;
    buffer->options.expand_tab = false;
    buffer->options.indent_width = 8;
    buffer->options.tab_width = 8;

    char *indent = make_indent(buffer, 16);
    EXPECT_STREQ(indent, "\t\t");
    free(indent);

    indent = make_indent(buffer, 17);
    EXPECT_STREQ(indent, "\t\t ");
    free(indent);

    indent = make_indent(buffer, 20);
    EXPECT_STREQ(indent, "\t\t    ");
    free(indent);

    indent = make_indent(buffer, 24);
    EXPECT_STREQ(indent, "\t\t\t");
    free(indent);

    buffer->options.expand_tab = true;

    indent = make_indent(buffer, 8);
    EXPECT_STREQ(indent, "        ");
    free(indent);

    indent = make_indent(buffer, 7);
    EXPECT_STREQ(indent, "       ");
    free(indent);

    buffer->options = saved_opts;
}

static const TestEntry tests[] = {
    TEST(test_find_buffer_by_id),
    TEST(test_make_indent),
};

const TestGroup buffer_tests = TEST_GROUP(tests);
