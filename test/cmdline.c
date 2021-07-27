#include <stdlib.h>
#include "test.h"
#include "cmdline.h"
#include "completion.h"
#include "editor.h"
#include "mode.h"

#define EXPECT_STRING_EQ(s, cstr) \
    EXPECT_STREQ(string_borrow_cstring(&(s)), (cstr))

static void test_command_mode(void)
{
    const CommandSet *cmds = &cmd_mode_commands;
    CommandLine *c = &editor.cmdline;
    set_input_mode(INPUT_COMMAND);
    EXPECT_EQ(editor.input_mode, INPUT_COMMAND);

    handle_input('a');
    EXPECT_EQ(c->pos, 1);
    EXPECT_STRING_EQ(c->buf, "a");

    handle_input(0x1F999);
    EXPECT_EQ(c->pos, 5);
    EXPECT_STRING_EQ(c->buf, "a\xF0\x9F\xA6\x99");

    // Delete at end-of-line should do nothing
    handle_command(cmds, "delete", false);
    EXPECT_EQ(c->pos, 5);
    EXPECT_EQ(c->buf.len, 5);

    handle_command(cmds, "erase", false);
    EXPECT_EQ(c->pos, 1);
    EXPECT_STRING_EQ(c->buf, "a");

    handle_command(cmds, "erase", false);
    EXPECT_EQ(c->pos, 0);
    EXPECT_STRING_EQ(c->buf, "");

    cmdline_set_text(c, "word1 word2 word3 word4");
    handle_command(cmds, "eol", false);
    EXPECT_EQ(c->pos, 23);

    handle_command(cmds, "erase-word", false);
    EXPECT_EQ(c->pos, 18);
    EXPECT_STRING_EQ(c->buf, "word1 word2 word3 ");

    handle_command(cmds, "erase-word", false);
    EXPECT_EQ(c->pos, 12);
    EXPECT_STRING_EQ(c->buf, "word1 word2 ");

    handle_command(cmds, "word-bwd", false);
    EXPECT_EQ(c->pos, 6);

    handle_command(cmds, "word-fwd", false);
    EXPECT_EQ(c->pos, 12);

    handle_command(cmds, "bol", false);
    EXPECT_EQ(c->pos, 0);

    handle_command(cmds, "delete-word", false);
    EXPECT_EQ(c->pos, 0);
    EXPECT_STRING_EQ(c->buf, "word2 ");

    handle_command(cmds, "right", false);
    EXPECT_EQ(c->pos, 1);

    handle_command(cmds, "erase-bol", false);
    EXPECT_EQ(c->pos, 0);
    EXPECT_STRING_EQ(c->buf, "ord2 ");

    handle_command(cmds, "delete", false);
    EXPECT_EQ(c->pos, 0);
    EXPECT_STRING_EQ(c->buf, "rd2 ");

    handle_command(cmds, "delete-eol", false);
    EXPECT_EQ(c->pos, 0);
    EXPECT_EQ(c->buf.len, 0);

    // Left at beginning-of-line should do nothing
    handle_command(cmds, "left", false);
    EXPECT_EQ(c->pos, 0);

    cmdline_set_text(c, "...");
    EXPECT_EQ(c->pos, 3);
    EXPECT_EQ(c->buf.len, 3);

    handle_command(cmds, "bol", false);
    EXPECT_EQ(c->pos, 0);
    EXPECT_EQ(c->buf.len, 3);

    handle_command(cmds, "right", false);
    EXPECT_EQ(c->pos, 1);

    handle_command(cmds, "word-bwd", false);
    EXPECT_EQ(c->pos, 0);

    handle_command(cmds, "eol", false);
    EXPECT_EQ(c->pos, 3);

    handle_command(cmds, "cancel", false);
    EXPECT_EQ(c->pos, 0);
    EXPECT_NULL(c->search_pos);
    EXPECT_EQ(c->buf.len, 0);
    EXPECT_EQ(editor.input_mode, INPUT_NORMAL);

    string_free(&c->buf);
    EXPECT_NULL(c->buf.buffer);
    EXPECT_EQ(c->buf.alloc, 0);
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

    cmdline_set_text(&editor.cmdline, "set esc-timeout ");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "set esc-timeout 100 ");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "set x y tab-b");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "set x y tab-bar ");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "set x y tab-bar true ");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "save -u test/data/");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "save -u test/data/3lines.txt");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "include -b r");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "include -b rc ");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "option gitcom");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "option gitcommit ");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "option gitcommit auto-indent");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "errorfmt c r ");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "errorfmt c r _");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "errorfmt c r column");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "repeat 3 ");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "repeat 3 alias");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "repeat 3 hi ");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "repeat 3 hi activetab");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "left; right; word-");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "left; right; word-bwd");
    reset_completion();
}

// This should only be run after init_headless_mode() because the completions
// depend on the buffer and default config being initialized
static void test_complete_command_extra(void)
{
    cmdline_set_text(&editor.cmdline, "show bi");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "show bind ");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "show bind C-?");
    complete_command_prev();
    EXPECT_STRING_EQ(editor.cmdline.buf, "show bind up");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "show errorfmt gc");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "show errorfmt gcc ");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "option c expand-tab ");
    complete_command_next();
    EXPECT_STRING_EQ(editor.cmdline.buf, "option c expand-tab true ");
    reset_completion();
}

static const TestEntry tests[] = {
    TEST(test_command_mode),
    TEST(test_complete_command),
};

static const TestEntry tests_late[] = {
    TEST(test_complete_command_extra),
};

const TestGroup cmdline_tests = TEST_GROUP(tests);
const TestGroup cmdline_tests_late = TEST_GROUP(tests_late);
