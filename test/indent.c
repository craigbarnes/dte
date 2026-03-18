#include "test.h"
#include "block-iter.h"
#include "buffer.h"
#include "editor.h"
#include "indent.h"
#include "options.h"
#include "view.h"
#include "window.h"

static void test_get_indent_info(TestContext *ctx)
{
    LocalOptions options = {
        .expand_tab = true,
        .indent_width = 4,
        .tab_width = 4,
    };

    StringView line = strview("            ");
    IndentInfo info = get_indent_info(&options, line);
    EXPECT_EQ(info.bytes, 12);
    EXPECT_EQ(info.width, 12);
    EXPECT_EQ(info.level, 3);
    EXPECT_TRUE(info.wsonly);
    EXPECT_TRUE(info.sane);

    line = strview("\t\t");
    info = get_indent_info(&options, line);
    EXPECT_EQ(info.bytes, 2);
    EXPECT_EQ(info.width, 8);
    EXPECT_EQ(info.level, 2);
    EXPECT_TRUE(info.wsonly);
    EXPECT_FALSE(info.sane);

    line = strview("    xyz");
    info = get_indent_info(&options, line);
    EXPECT_EQ(info.bytes, 4);
    EXPECT_EQ(info.width, 4);
    EXPECT_EQ(info.level, 1);
    EXPECT_FALSE(info.wsonly);
    EXPECT_TRUE(info.sane);

    options.expand_tab = false;
    line = strview("\t\t");
    info = get_indent_info(&options, line);
    EXPECT_EQ(info.bytes, 2);
    EXPECT_EQ(info.width, 8);
    EXPECT_EQ(info.level, 2);
    EXPECT_TRUE(info.wsonly);
    EXPECT_TRUE(info.sane);

    line = strview("    test");
    info = get_indent_info(&options, line);
    EXPECT_EQ(info.bytes, 4);
    EXPECT_EQ(info.width, 4);
    EXPECT_EQ(info.level, 1);
    EXPECT_FALSE(info.wsonly);
    EXPECT_FALSE(info.sane);

    options.indent_width = 8;
    options.tab_width = 8;
    line = strview("\t\t  ");
    info = get_indent_info(&options, line);
    EXPECT_EQ(info.bytes, 4);
    EXPECT_EQ(info.width, 18);
    EXPECT_EQ(info.level, 2);
    EXPECT_TRUE(info.wsonly);
    EXPECT_TRUE(info.sane);

    line = strview("\t \t ");
    info = get_indent_info(&options, line);
    EXPECT_EQ(info.bytes, 4);
    EXPECT_EQ(info.width, 17);
    EXPECT_EQ(info.level, 2);
    EXPECT_TRUE(info.wsonly);
    EXPECT_FALSE(info.sane);

    line = strview("    test");
    info = get_indent_info(&options, line);
    EXPECT_EQ(info.bytes, 4);
    EXPECT_EQ(info.width, 4);
    EXPECT_EQ(info.level, 0);
    EXPECT_FALSE(info.wsonly);
    EXPECT_TRUE(info.sane);

    line = strview("        test");
    info = get_indent_info(&options, line);
    EXPECT_EQ(info.bytes, 8);
    EXPECT_EQ(info.width, 8);
    EXPECT_EQ(info.level, 1);
    EXPECT_FALSE(info.wsonly);
    EXPECT_FALSE(info.sane);
}

static void test_indent_level(TestContext *ctx)
{
    EXPECT_EQ(indent_level(0, 2), 0);
    EXPECT_EQ(indent_level(1, 2), 0);
    EXPECT_EQ(indent_level(2, 2), 1);
    EXPECT_EQ(indent_level(3, 2), 1);
    EXPECT_EQ(indent_level(4, 2), 2);
    EXPECT_EQ(indent_level(7, 8), 0);
    EXPECT_EQ(indent_level(8, 8), 1);
    EXPECT_EQ(indent_level(9, 8), 1);

    EXPECT_EQ(indent_remainder(0, 2), 0);
    EXPECT_EQ(indent_remainder(1, 2), 1);
    EXPECT_EQ(indent_remainder(2, 2), 0);
    EXPECT_EQ(indent_remainder(3, 2), 1);
    EXPECT_EQ(indent_remainder(4, 2), 0);
    EXPECT_EQ(indent_remainder(7, 8), 7);
    EXPECT_EQ(indent_remainder(8, 8), 0);
    EXPECT_EQ(indent_remainder(9, 8), 1);

    for (size_t x = 0; x <= 17; x++) {
        for (size_t m = 1; m <= 8; m++) {
            EXPECT_EQ(indent_level(x, m), x / m);
            EXPECT_EQ(indent_remainder(x, m), x % m);
        }
    }
}

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

static void test_indent_lines(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    View *view = window_open_empty_buffer(e->window);
    LocalOptions *opts = &view->buffer->options;
    opts->indent_width = 4;
    opts->auto_indent = false;
    opts->emulate_tab = true;
    opts->expand_tab = true;

    static const char text[] = "    line 1\n    line 2\n";
    BlockIter *cursor = &view->cursor;
    buffer_insert_bytes(view, text, sizeof(text) - 1);
    block_iter_goto_offset(cursor, 4);
    EXPECT_EQ(block_iter_get_offset(cursor), 4);

    CodePoint u = 0;
    EXPECT_EQ(block_iter_get_char(cursor, &u), 1);
    EXPECT_EQ(u, 'l');

    indent_lines(view, 0);
    EXPECT_EQ(block_iter_get_offset(cursor), 4);
    indent_lines(view, 1);
    EXPECT_EQ(block_iter_get_offset(cursor), 8);
    indent_lines(view, 1);
    EXPECT_EQ(block_iter_get_offset(cursor), 12);
    indent_lines(view, 2);
    EXPECT_EQ(block_iter_get_offset(cursor), 20);
    indent_lines(view, -1);
    EXPECT_EQ(block_iter_get_offset(cursor), 16);
    indent_lines(view, -2);
    EXPECT_EQ(block_iter_get_offset(cursor), 8);
    indent_lines(view, -2);
    EXPECT_EQ(block_iter_get_offset(cursor), 0);
    indent_lines(view, -1);
    EXPECT_EQ(block_iter_get_offset(cursor), 0);
    indent_lines(view, 1);
    EXPECT_EQ(block_iter_get_offset(cursor), 4);
    indent_lines(view, -50);
    EXPECT_EQ(block_iter_get_offset(cursor), 0);

    window_close_current_view(e->window);
}

static const TestEntry tests[] = {
    TEST(test_get_indent_info),
    TEST(test_indent_level),
    TEST(test_next_indent_width),
};

static const TestEntry tests_late[] = {
    TEST(test_indent_lines),
};

const TestGroup indent_tests = TEST_GROUP(tests);
const TestGroup indent_tests_late = TEST_GROUP(tests_late);
