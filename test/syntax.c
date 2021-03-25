#include <string.h>
#include "test.h"
#include "block-iter.h"
#include "config.h"
#include "syntax/bitset.h"
#include "syntax/highlight.h"
#include "util/debug.h"
#include "util/utf8.h"
#include "window.h"

static void test_bitset(void)
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

static void test_hl_line(void)
{
    if (!get_builtin_config("syntax/c")) {
        DEBUG_LOG("syntax/c not available; skipping %s()", __func__);
        return;
    }

    const Encoding enc = encoding_from_type(UTF8);
    EXPECT_EQ(enc.type, UTF8);
    EXPECT_STREQ(enc.name, "UTF-8");
    View *v = window_open_file(window, "test/data/test.c", &enc);
    ASSERT_NONNULL(v);

    const size_t line_nr = 5;
    ASSERT_TRUE(v->buffer->nl >= line_nr);
    hl_fill_start_states(v->buffer, v->buffer->nl);
    block_iter_goto_line(&v->cursor, line_nr - 1);
    view_update_cursor_x(v);
    view_update_cursor_y(v);
    view_update(v);
    ASSERT_EQ(v->cx, 0);
    ASSERT_EQ(v->cy, line_nr - 1);

    StringView line;
    fetch_this_line(&v->cursor, &line);
    ASSERT_EQ(line.length, 56);

    bool next_changed;
    TermColor **colors = hl_line(buffer, &line, line_nr, &next_changed);
    EXPECT_TRUE(next_changed);

    const TermColor *t = find_color("text");
    const TermColor *c = find_color("constant");
    const TermColor *s = find_color("string");
    const TermColor *x = find_color("special");
    const TermColor *n = find_color("numeric");
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
        if (i >= ARRAY_COUNT(expected_colors)) {
            continue;
        }
        IEXPECT_TRUE(same_color(colors[i], expected_colors[i]));
    }

    EXPECT_EQ(i, ARRAY_COUNT(expected_colors));
    window_close_current();
}

static const TestEntry tests[] = {
    TEST(test_bitset),
    TEST(test_hl_line),
};

const TestGroup syntax_tests = TEST_GROUP(tests);
