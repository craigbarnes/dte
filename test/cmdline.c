#include <stdlib.h>
#include "test.h"
#include "cmdline.h"
#include "completion.h"
#include "editor.h"

#define EXPECT_STRING_EQ(s, cstr) \
    EXPECT_STREQ(string_borrow_cstring(&(s)), (cstr))

static void test_cmdline_handle_key(void)
{
    CommandLine *c = &editor.cmdline;
    History *h = &editor.command_history;

    int ret = cmdline_handle_key(c, h, 'a');
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 1);
    EXPECT_STRING_EQ(c->buf, "a");

    ret = cmdline_handle_key(c, h, 0x1F999);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 5);
    EXPECT_STRING_EQ(c->buf, "a\xF0\x9F\xA6\x99");

    // Delete at end-of-line should do nothing
    ret = cmdline_handle_key(c, h, KEY_DELETE);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 5);
    EXPECT_EQ(c->buf.len, 5);

    ret = cmdline_handle_key(c, h, CTRL('H'));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 1);
    EXPECT_STRING_EQ(c->buf, "a");

    ret = cmdline_handle_key(c, h, CTRL('?'));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);
    EXPECT_STRING_EQ(c->buf, "");

    cmdline_set_text(c, "word1 word2 word3 word4");
    ret = cmdline_handle_key(c, h, KEY_END);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 23);

    ret = cmdline_handle_key(c, h, MOD_META | MOD_CTRL | 'H');
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 18);
    EXPECT_STRING_EQ(c->buf, "word1 word2 word3 ");

    ret = cmdline_handle_key(c, h, MOD_META | MOD_CTRL | '?');
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 12);
    EXPECT_STRING_EQ(c->buf, "word1 word2 ");

    ret = cmdline_handle_key(c, h, CTRL(KEY_LEFT));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 6);

    ret = cmdline_handle_key(c, h, CTRL(KEY_RIGHT));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 12);

    ret = cmdline_handle_key(c, h, KEY_HOME);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);

    ret = cmdline_handle_key(c, h, MOD_META | KEY_DELETE);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);
    EXPECT_STRING_EQ(c->buf, "word2 ");

    ret = cmdline_handle_key(c, h, KEY_RIGHT);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 1);

    ret = cmdline_handle_key(c, h, CTRL('U'));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);
    EXPECT_STRING_EQ(c->buf, "ord2 ");

    ret = cmdline_handle_key(c, h, KEY_DELETE);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);
    EXPECT_STRING_EQ(c->buf, "rd2 ");

    ret = cmdline_handle_key(c, h, CTRL('K'));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);
    EXPECT_EQ(c->buf.len, 0);

    // Left arrow at beginning-of-line should do nothing
    ret = cmdline_handle_key(c, h, KEY_LEFT);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);

    cmdline_set_text(c, "...");
    EXPECT_EQ(c->pos, 3);
    EXPECT_EQ(c->buf.len, 3);

    ret = cmdline_handle_key(c, h, CTRL('A'));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);
    EXPECT_EQ(c->buf.len, 3);

    ret = cmdline_handle_key(c, h, KEY_RIGHT);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 1);

    ret = cmdline_handle_key(c, h, CTRL(KEY_LEFT));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);

    ret = cmdline_handle_key(c, h, CTRL('E'));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 3);

    ret = cmdline_handle_key(c, h, CTRL('G'));
    EXPECT_EQ(ret, CMDLINE_CANCEL);
    EXPECT_EQ(c->pos, 0);
    EXPECT_EQ(c->buf.len, 0);
}

#define ENV_VAR_PREFIX "D__p_tYmz3_"
#define ENV_VAR_NAME ENV_VAR_PREFIX "_VAR"

static void test_complete_command(void)
{
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "alias");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "wrap");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "wrap-paragraph ");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "open test/data/.e");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "open test/data/.editorconfig ");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "toggle ");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "toggle auto-indent");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "set expand-tab f");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "set expand-tab false ");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "set case-sensitive-search a");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "set case-sensitive-search auto ");
    reset_completion();

    ASSERT_EQ(setenv(ENV_VAR_NAME, "xyz", true), 0);
    cmdline_set_text(&editor.cmdline, "insert $" ENV_VAR_PREFIX);
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "insert $" ENV_VAR_NAME);
    reset_completion();
    ASSERT_EQ(unsetenv(ENV_VAR_NAME), 0);

    cmdline_clear(&editor.cmdline);
    complete_command_prev();
    EXPECT_STRING_EQ(editor.cmdline.buf, "alias");
    complete_command_prev();
    EXPECT_STRING_EQ(editor.cmdline.buf, "wswap");
    complete_command_prev();
    EXPECT_STRING_EQ(editor.cmdline.buf, "wsplit");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "hi default ");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "hi default black");
    complete_command_prev();
    EXPECT_STRING_EQ(editor.cmdline.buf, "hi default yellow");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "hi default black");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "hi default blink");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "show op");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "show option ");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "show option auto-indent");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "set ws-error tr");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "set ws-error trailing ");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "set ws-error special,tab-");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "set ws-error special,tab-after-indent");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "set ws-error special,tab-indent");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "set ws-error special,tab-after-indent");
    reset_completion();
}

static const TestEntry tests[] = {
    TEST(test_cmdline_handle_key),
    TEST(test_complete_command),
};

const TestGroup cmdline_tests = TEST_GROUP(tests);
