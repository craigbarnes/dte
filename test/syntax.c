#include "test.h"
#include "block-iter.h"
#include "config.h"
#include "editor.h"
#include "syntax/bitset.h"
#include "syntax/highlight.h"
#include "util/log.h"
#include "util/utf8.h"
#include "window.h"

static void test_bitset(TestContext *ctx)
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

static void test_hl_line(TestContext *ctx)
{
    if (!get_builtin_config("syntax/c")) {
        LOG_INFO("syntax/c not available; skipping %s()", __func__);
        return;
    }

    const Encoding utf8 = encoding_from_type(UTF8);
    EXPECT_EQ(utf8.type, UTF8);
    EXPECT_STREQ(utf8.name, "UTF-8");

    EditorState *e = ctx->userdata;
    Window *window = e->window;
    ASSERT_NONNULL(window);
    View *view = window_open_file(window, "test/data/test.c", &utf8);
    ASSERT_NONNULL(view);
    Buffer *buffer = view->buffer;
    ASSERT_NONNULL(buffer);

    const ColorScheme *colors = &e->colors;
    const size_t line_nr = 5;
    ASSERT_TRUE(buffer->nl >= line_nr);
    hl_fill_start_states(buffer, colors, buffer->nl);
    block_iter_goto_line(&view->cursor, line_nr - 1);
    view_update_cursor_x(view);
    view_update_cursor_y(view);
    view_update(view, 0);
    ASSERT_EQ(view->cx, 0);
    ASSERT_EQ(view->cy, line_nr - 1);

    StringView line;
    fetch_this_line(&view->cursor, &line);
    ASSERT_EQ(line.length, 56);

    bool next_changed;
    const TermColor **hl = hl_line(buffer, colors, &line, line_nr, &next_changed);
    ASSERT_NONNULL(hl);
    EXPECT_TRUE(next_changed);

    const TermColor *t = find_color(colors, "text");
    const TermColor *c = find_color(colors, "constant");
    const TermColor *s = find_color(colors, "string");
    const TermColor *x = find_color(colors, "special");
    const TermColor *n = find_color(colors, "numeric");
    ASSERT_NONNULL(t);
    ASSERT_NONNULL(c);
    ASSERT_NONNULL(s);
    ASSERT_NONNULL(x);
    ASSERT_NONNULL(n);

    const TermColor *const expected_colors[] = {
        t, t, t, t, t, t, t, t, t, t, t, t, c, c, c, c,
        c, c, t, t, s, s, s, s, s, s, s, s, s, s, s, s,
        s, s, s, x, x, s, t, t, s, s, s, s, s, t, t, x,
        x, x, t, t, n, n, t, t
    };

    size_t i = 0;
    for (size_t pos = 0; pos < line.length; i++) {
        CodePoint u = u_get_char(line.data, line.length, &pos);
        IEXPECT_EQ(u, line.data[i]);
        if (i >= ARRAYLEN(expected_colors)) {
            continue;
        }
        IEXPECT_TRUE(same_color(hl[i], expected_colors[i]));
    }

    EXPECT_EQ(i, ARRAYLEN(expected_colors));
    window_close(window);
}

static const TestEntry tests[] = {
    TEST(test_bitset),
    TEST(test_hl_line),
};

const TestGroup syntax_tests = TEST_GROUP(tests);
