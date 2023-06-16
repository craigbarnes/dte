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

static void test_buffer_mark_lines_changed(TestContext *ctx)
{
    Buffer b = {
        .changed_line_min = 101,
        .changed_line_max = 399,
    };

    buffer_mark_lines_changed(&b, 400, 12);
    EXPECT_EQ(b.changed_line_min, 12);
    EXPECT_EQ(b.changed_line_max, 400);

    buffer_mark_lines_changed(&b, 3, 3);
    EXPECT_EQ(b.changed_line_min, 3);
    EXPECT_EQ(b.changed_line_max, 400);

    buffer_mark_lines_changed(&b, 3, 990);
    EXPECT_EQ(b.changed_line_min, 3);
    EXPECT_EQ(b.changed_line_max, 990);

    buffer_mark_lines_changed(&b, 1234, 1);
    EXPECT_EQ(b.changed_line_min, 1);
    EXPECT_EQ(b.changed_line_max, 1234);
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

static void test_buffer_insert_bytes(TestContext *ctx)
{
    size_t len = 600; // (> BLOCK_EDIT_SIZE)
    char *text = xmalloc(len);
    memset(text, 'a', len);
    text[len - 1] = '\n';
    for (size_t i = 72; i < len; i += 72) {
        text[i] = '\n';
    }

    EditorState *e = ctx->userdata;
    View *view = window_open_empty_buffer(e->window);
    const Buffer *buffer = view->buffer;
    uintmax_t counts[2];
    buffer_count_blocks_and_bytes(buffer, counts);
    EXPECT_EQ(counts[0], 1);
    EXPECT_EQ(counts[1], 0);
    EXPECT_EQ(buffer->nl, 0);

    buffer_insert_bytes(view, text, len);
    buffer_count_blocks_and_bytes(buffer, counts);
    EXPECT_EQ(counts[0], 2);
    EXPECT_EQ(counts[1], len);
    EXPECT_EQ(buffer->nl, 9);

    block_iter_goto_offset(&view->cursor, len / 2);
    EXPECT_EQ(view->cursor.offset, 300);
    buffer_insert_bytes(view, text, len);
    EXPECT_EQ(view->cursor.offset, 300);
    buffer_count_blocks_and_bytes(buffer, counts);
    EXPECT_EQ(counts[0], 4);
    EXPECT_EQ(counts[1], len * 2);
    EXPECT_EQ(buffer->nl, 18);

    free(text);
    window_close_current_view(e->window);
}

static const TestEntry tests[] = {
    TEST(test_find_buffer_by_id),
    TEST(test_buffer_mark_lines_changed),
    TEST(test_make_indent),
    TEST(test_get_indent_for_next_line),
    TEST(test_buffer_insert_bytes),
};

const TestGroup buffer_tests = TEST_GROUP(tests);
