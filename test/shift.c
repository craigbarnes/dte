#include "test.h"
#include "block-iter.h"
#include "buffer.h"
#include "editor.h"
#include "options.h"
#include "shift.h"
#include "view.h"
#include "window.h"

static void test_shift_lines(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    View *view = window_open_empty_buffer(e->window);
    LocalOptions *opts = &view->buffer->options;
    opts->indent_width = 4;
    opts->auto_indent = false;
    opts->emulate_tab = true;
    opts->expand_tab = true;

    static const char text[] = "    line 1\n    line 2\n";
    buffer_insert_bytes(view, text, sizeof(text) - 1);
    block_iter_goto_offset(&view->cursor, 4);
    EXPECT_EQ(block_iter_get_offset(&view->cursor), 4);

    CodePoint u = 0;
    EXPECT_EQ(block_iter_get_char(&view->cursor, &u), 1);
    EXPECT_EQ(u, 'l');

    shift_lines(view, 1);
    EXPECT_EQ(block_iter_get_offset(&view->cursor), 8);
    shift_lines(view, 1);
    EXPECT_EQ(block_iter_get_offset(&view->cursor), 12);
    shift_lines(view, 2);
    EXPECT_EQ(block_iter_get_offset(&view->cursor), 20);
    shift_lines(view, -1);
    EXPECT_EQ(block_iter_get_offset(&view->cursor), 16);
    shift_lines(view, -2);
    EXPECT_EQ(block_iter_get_offset(&view->cursor), 8);
    shift_lines(view, -2);
    EXPECT_EQ(block_iter_get_offset(&view->cursor), 0);
    shift_lines(view, -1);
    EXPECT_EQ(block_iter_get_offset(&view->cursor), 0);
    shift_lines(view, 1);
    EXPECT_EQ(block_iter_get_offset(&view->cursor), 4);
    shift_lines(view, -50);
    EXPECT_EQ(block_iter_get_offset(&view->cursor), 0);

    window_close_current_view(e->window);
}

static const TestEntry tests[] = {
    TEST(test_shift_lines),
};

const TestGroup shift_tests = TEST_GROUP(tests);
