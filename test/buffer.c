#include <stdlib.h>
#include "test.h"
#include "buffer.h"
#include "editor.h"
#include "indent.h"
#include "regexp.h"

static void test_find_buffer_by_id(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    const Buffer *buffer = e->buffer;
    ASSERT_NONNULL(buffer);
    EXPECT_PTREQ(find_buffer_by_id(&e->buffers, buffer->id), buffer);

    const unsigned long large_id = 1UL << 30;
    static_assert_compatible_types(large_id, buffer->id);
    EXPECT_NULL(find_buffer_by_id(&e->buffers, large_id));
}

static void test_make_indent(TestContext *ctx)
{
    LocalOptions options = {
        .expand_tab = false,
        .indent_width = 8,
        .tab_width = 8,
    };

    char *indent = make_indent(&options, 16);
    EXPECT_STREQ(indent, "\t\t");
    free(indent);

    indent = make_indent(&options, 17);
    EXPECT_STREQ(indent, "\t\t ");
    free(indent);

    indent = make_indent(&options, 20);
    EXPECT_STREQ(indent, "\t\t    ");
    free(indent);

    indent = make_indent(&options, 24);
    EXPECT_STREQ(indent, "\t\t\t");
    free(indent);

    options.expand_tab = true;

    indent = make_indent(&options, 8);
    EXPECT_STREQ(indent, "        ");
    free(indent);

    indent = make_indent(&options, 7);
    EXPECT_STREQ(indent, "       ");
    free(indent);
}

static void test_get_indent_for_next_line(TestContext *ctx)
{
    const char *pattern = "\\{$";
    const InternedRegexp *ir = regexp_intern(pattern);
    ASSERT_NONNULL(ir);

    LocalOptions options = {
        .brace_indent = false,
        .expand_tab = true,
        .indent_regex = ir,
        .indent_width = 4,
        .tab_width = 8
    };

    const StringView line1 = STRING_VIEW("foo {");
    char *indent = get_indent_for_next_line(&options, &line1);
    EXPECT_STREQ(indent, "    ");
    free(indent);

    const StringView line2 = STRING_VIEW("foo");
    indent = get_indent_for_next_line(&options, &line2);
    EXPECT_STREQ(indent, NULL);
    free(indent);
}

static const TestEntry tests[] = {
    TEST(test_find_buffer_by_id),
    TEST(test_make_indent),
    TEST(test_get_indent_for_next_line),
};

const TestGroup buffer_tests = TEST_GROUP(tests);
