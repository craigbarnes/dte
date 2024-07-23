#include "test.h"
#include "block-iter.h"
#include "config.h"
#include "editor.h"
#include "encoding.h"
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

    EditorState *e = ctx->userdata;
    Window *window = e->window;
    ASSERT_NONNULL(window);
    View *view = window_open_file(window, "test/data/test.c", NULL);
    ASSERT_NONNULL(view);
    Buffer *buffer = view->buffer;
    ASSERT_NONNULL(buffer);
    const size_t line_nr = 5;
    ASSERT_TRUE(buffer->nl >= line_nr);

    Syntax *syn = buffer->syntax;
    ASSERT_NONNULL(syn);
    ASSERT_NONNULL(syn->start_state);
    EXPECT_STREQ(syn->name, "c");
    EXPECT_FALSE(syn->heredoc);

    const StyleMap *styles = &e->styles;
    PointerArray *lss = &buffer->line_start_states;
    BlockIter tmp = block_iter(buffer);
    hl_fill_start_states(syn, lss, styles, &tmp, buffer->nl);
    block_iter_goto_line(&view->cursor, line_nr - 1);
    view_update(view, 0);
    ASSERT_EQ(view->cx, 0);
    ASSERT_EQ(view->cy, line_nr - 1);

    StringView line;
    fetch_this_line(&view->cursor, &line);
    ASSERT_EQ(line.length, 65);

    bool next_changed;
    const TermStyle **hl = hl_line(syn, lss, styles, &line, line_nr, &next_changed);
    ASSERT_NONNULL(hl);
    EXPECT_TRUE(next_changed);

    const TermStyle *t = find_style(styles, "text");
    const TermStyle *c = find_style(styles, "constant");
    const TermStyle *s = find_style(styles, "string");
    const TermStyle *x = find_style(styles, "special");
    const TermStyle *n = find_style(styles, "numeric");
    const TermStyle *y = find_style(styles, "type");
    ASSERT_NONNULL(t);
    ASSERT_NONNULL(c);
    ASSERT_NONNULL(s);
    ASSERT_NONNULL(x);
    ASSERT_NONNULL(n);
    ASSERT_NONNULL(y);
    EXPECT_EQ(t->fg, COLOR_DEFAULT);
    EXPECT_EQ(c->fg, COLOR_CYAN);
    EXPECT_EQ(s->fg, COLOR_YELLOW);
    EXPECT_EQ(x->fg, COLOR_MAGENTA);
    EXPECT_EQ(n->fg, COLOR_BLUE);
    EXPECT_EQ(y->fg, COLOR_GREEN);

    const TermStyle *const expected_styles[] = {
        t, t, t, t, t, t, t, t, t, t, t, t, c, c, c, c,
        c, c, t, t, s, s, s, s, s, s, s, s, s, s, s, s,
        s, s, s, s, x, x, s, t, t, s, s, s, s, s, t, t,
        x, x, x, t, t, t, y, y, y, y, y, y, t, n, n, t,
        t
    };

    size_t i = 0;
    for (size_t pos = 0; pos < line.length; i++) {
        CodePoint u = u_get_char(line.data, line.length, &pos);
        IEXPECT_EQ(u, line.data[i]);
        if (i >= ARRAYLEN(expected_styles)) {
            continue;
        }
        IEXPECT_TRUE(same_style(hl[i], expected_styles[i]));
    }

    EXPECT_EQ(i, ARRAYLEN(expected_styles));
    window_close(window);
}

static const TestEntry tests[] = {
    TEST(test_bitset),
    TEST(test_hl_line),
};

const TestGroup syntax_tests = TEST_GROUP(tests);
