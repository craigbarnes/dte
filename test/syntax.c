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
    bitset_add_char_range(set, "\1-?r-r^-b");
    EXPECT_TRUE(bitset_contains(set, '\1'));
    EXPECT_TRUE(bitset_contains(set, '\2'));
    EXPECT_TRUE(bitset_contains(set, ' '));
    EXPECT_TRUE(bitset_contains(set, '!'));
    EXPECT_TRUE(bitset_contains(set, '>'));
    EXPECT_TRUE(bitset_contains(set, '?'));
    EXPECT_TRUE(bitset_contains(set, 'r'));
    EXPECT_TRUE(bitset_contains(set, '^'));
    EXPECT_TRUE(bitset_contains(set, '_'));
    EXPECT_TRUE(bitset_contains(set, '`'));
    EXPECT_TRUE(bitset_contains(set, 'a'));
    EXPECT_TRUE(bitset_contains(set, 'b'));
    EXPECT_FALSE(bitset_contains(set, '\0'));
    EXPECT_FALSE(bitset_contains(set, '@'));
    EXPECT_FALSE(bitset_contains(set, 'c'));
    EXPECT_FALSE(bitset_contains(set, 'q'));
    EXPECT_FALSE(bitset_contains(set, 's'));
    EXPECT_FALSE(bitset_contains(set, 'A'));

    memset(set, 0, sizeof(set));
    bitset_add_char_range(set, "\x03-\xFC");
    EXPECT_TRUE(bitset_contains(set, '\x03'));
    EXPECT_TRUE(bitset_contains(set, '\x40'));
    EXPECT_TRUE(bitset_contains(set, '\x7F'));
    EXPECT_TRUE(bitset_contains(set, '\x80'));
    EXPECT_TRUE(bitset_contains(set, '\xFC'));
    EXPECT_FALSE(bitset_contains(set, '\x00'));
    EXPECT_FALSE(bitset_contains(set, '\x01'));
    EXPECT_FALSE(bitset_contains(set, '\x02'));
    EXPECT_FALSE(bitset_contains(set, '\xFE'));
    EXPECT_FALSE(bitset_contains(set, '\xFF'));
    for (unsigned int i = 3; i <= 0xFC; i++) {
        IEXPECT_TRUE(bitset_contains(set, i));
    }

    memset(set, 0, sizeof(set));
    bitset_add_char_range(set, "?-@");
    EXPECT_TRUE(bitset_contains(set, '?'));
    EXPECT_TRUE(bitset_contains(set, '@'));
    EXPECT_FALSE(bitset_contains(set, '>'));
    EXPECT_FALSE(bitset_contains(set, 'A'));

    memset(set, 0, sizeof(set));
    bitset_add_char_range(set, "z-a");
    FOR_EACH_I(i, set) {
        EXPECT_UINT_EQ(set[i], 0);
    }

    BITSET_INVERT(set);
    FOR_EACH_I(i, set) {
        EXPECT_UINT_EQ(set[i], bitset_word_max());
    }
}

static void test_load_syntax_errors(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    ErrorBuffer *ebuf = &e->err;
    SyntaxLoadFlags flags = SYN_LINT;

    clear_error(ebuf);
    StringView text = strview("syntax dup; state a; eat this; syntax dup; state b; eat this");
    EXPECT_NULL(load_syntax(e, text, "dup", flags));
    EXPECT_STREQ(ebuf->buf, "dup:2: Syntax 'dup' already exists");

    clear_error(ebuf);
    text = strview("syntax empty");
    EXPECT_NULL(load_syntax(e, text, "empty", flags));
    EXPECT_STREQ(ebuf->buf, "empty:2: Empty syntax");

    clear_error(ebuf);
    text = strview("syntax hde; state a; heredocend b; eat this; state b; eat this");
    EXPECT_NULL(load_syntax(e, text, "hde", flags));
    EXPECT_STREQ(ebuf->buf, "hde:2: heredocend can be used only in subsyntaxes");

    clear_error(ebuf);
    text = strview("syntax loop; state ident; noeat this");
    EXPECT_NULL(load_syntax(e, text, "loop", flags));
    EXPECT_STREQ(ebuf->buf, "loop:1: noeat: using noeat to jump to same state causes infinite loop");

    clear_error(ebuf);
    text = strview("syntax ml; state a; inlist X this; eat this");
    EXPECT_NULL(load_syntax(e, text, "ml", flags));
    EXPECT_STREQ(ebuf->buf, "ml:2: No such list 'X'");

    clear_error(ebuf);
    text = strview("syntax mst; state a; noeat X");
    EXPECT_NULL(load_syntax(e, text, "mst", flags));
    EXPECT_STREQ(ebuf->buf, "mst:2: No such state 'X'");

    clear_error(ebuf);
    text = strview("syntax nda; state a; char a this");
    EXPECT_NULL(load_syntax(e, text, "nda", flags));
    EXPECT_STREQ(ebuf->buf, "nda:2: No default action in state 'a'");

    clear_error(ebuf);
    text = strview("syntax not-known; state a; eat this");
    EXPECT_NULL(load_syntax(e, text, "known", flags));
    EXPECT_STREQ(ebuf->buf, "known: no main syntax found (i.e. with name 'known')");

    // Non-fatal errors:

    // Unreachable state
    clear_error(ebuf);
    text = strview("syntax ust; state a; eat this; state U; eat a");
    const Syntax *syntax = load_syntax(e, text, "ust", flags);
    ASSERT_NONNULL(syntax);
    EXPECT_STREQ(syntax->name, "ust");
    EXPECT_EQ(syntax->states.count, 2);
    EXPECT_STREQ(syntax->start_state->name, "a");
    EXPECT_STREQ(ebuf->buf, "ust:2: State 'U' is unreachable");

    // Unused list
    clear_error(ebuf);
    text = strview("syntax ul; state a; eat this; list unused a");
    syntax = load_syntax(e, text, "ul", flags);
    ASSERT_NONNULL(syntax);
    EXPECT_STREQ(syntax->name, "ul");
    EXPECT_EQ(syntax->states.count, 1);
    EXPECT_STREQ(syntax->start_state->name, "a");
    EXPECT_STREQ(ebuf->buf, "ul:2: List 'unused' never used");

    // Unused sub-syntax
    clear_error(ebuf);
    text = strview("syntax .uss-x; state a; eat this; syntax uss; state a; eat this");
    syntax = load_syntax(e, text, "uss", flags);
    ASSERT_NONNULL(syntax);
    EXPECT_STREQ(syntax->name, "uss");
    EXPECT_EQ(syntax->states.count, 1);
    EXPECT_STREQ(syntax->start_state->name, "a");
    EXPECT_STREQ(ebuf->buf, "uss:2: Subsyntax '.uss-x' is unused");

    // Use of named destination, instead of "this"
    clear_error(ebuf);
    text = strview("syntax td; state a; eat a");
    syntax = load_syntax(e, text, "td", flags);
    ASSERT_NONNULL(syntax);
    EXPECT_STREQ(syntax->name, "td");
    EXPECT_EQ(syntax->states.count, 1);
    EXPECT_STREQ(syntax->start_state->name, "a");
    EXPECT_STREQ(ebuf->buf, "td:1: eat: destination 'a' can be optimized to 'this' in 'td' syntax");

    // Redundant emit-name
    clear_error(ebuf);
    text = strview("syntax ren; state a; eat this a");
    syntax = load_syntax(e, text, "ren", flags);
    ASSERT_NONNULL(syntax);
    EXPECT_STREQ(syntax->name, "ren");
    EXPECT_EQ(syntax->states.count, 1);
    EXPECT_STREQ(syntax->start_state->name, "a");
    EXPECT_STREQ(ebuf->buf, "ren:1: eat: emit-name 'a' not needed (destination state uses same emit-name)");
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
    view_update(view);
    ASSERT_EQ(view->cx, 0);
    ASSERT_EQ(view->cy, line_nr - 1);

    StringView line = get_current_line(&view->cursor);
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
    TEST(test_load_syntax_errors),
    TEST(test_hl_line),
};

const TestGroup syntax_tests = TEST_GROUP(tests);
