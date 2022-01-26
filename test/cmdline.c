#include <stdlib.h>
#include "test.h"
#include "cmdline.h"
#include "commands.h"
#include "completion.h"
#include "editor.h"
#include "mode.h"

#define EXPECT_STRING_EQ(s, cstr) \
    EXPECT_STREQ(string_borrow_cstring(&(s)), (cstr))

static void test_command_mode(void)
{
    const CommandSet *cmds = &cmd_mode_commands;
    CommandLine *c = &editor.cmdline;
    set_input_mode(&editor, INPUT_COMMAND);
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
    CommandLine *c = &editor.cmdline;
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "alias");
    reset_completion(c);

    cmdline_set_text(c, "wrap");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "wrap-paragraph ");
    reset_completion(c);

    cmdline_set_text(c, "open test/data/.e");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "open test/data/.editorconfig ");
    reset_completion(c);

    cmdline_set_text(c, "open GNUma");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "open GNUmakefile ");
    reset_completion(c);

    cmdline_set_text(c, "wsplit -bhr test/data/.e");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "wsplit -bhr test/data/.editorconfig ");
    reset_completion(c);

    cmdline_set_text(c, "toggle ");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "toggle auto-indent");
    reset_completion(c);

    cmdline_set_text(c, "set expand-tab f");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "set expand-tab false ");
    reset_completion(c);

    cmdline_set_text(c, "set case-sensitive-search a");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "set case-sensitive-search auto ");
    reset_completion(c);

    ASSERT_EQ(setenv(ENV_VAR_NAME, "xyz", true), 0);

    cmdline_set_text(c, "insert $" ENV_VAR_PREFIX);
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "insert $" ENV_VAR_NAME);
    reset_completion(c);

    cmdline_set_text(c, "setenv " ENV_VAR_PREFIX);
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "setenv " ENV_VAR_NAME " ");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "setenv " ENV_VAR_NAME " xyz ");
    reset_completion(c);

    ASSERT_EQ(unsetenv(ENV_VAR_NAME), 0);

    cmdline_clear(c);
    complete_command_prev(c);
    EXPECT_STRING_EQ(c->buf, "alias");
    complete_command_prev(c);
    EXPECT_STRING_EQ(c->buf, "wswap");
    complete_command_prev(c);
    EXPECT_STRING_EQ(c->buf, "wsplit");
    reset_completion(c);

    cmdline_set_text(c, "hi default ");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "hi default black");
    complete_command_prev(c);
    EXPECT_STRING_EQ(c->buf, "hi default yellow");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "hi default black");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "hi default blink");
    reset_completion(c);

    cmdline_set_text(c, "show op");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "show option ");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "show option auto-indent");
    reset_completion(c);

    cmdline_set_text(c, "set ws-error tr");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "set ws-error trailing ");
    reset_completion(c);

    cmdline_set_text(c, "set ws-error special,tab-");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "set ws-error special,tab-after-indent");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "set ws-error special,tab-indent");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "set ws-error special,tab-after-indent");
    reset_completion(c);

    cmdline_set_text(c, "set esc-timeout ");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "set esc-timeout 100 ");
    reset_completion(c);

    cmdline_set_text(c, "set x y tab-b");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "set x y tab-bar ");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "set x y tab-bar true ");
    reset_completion(c);

    cmdline_set_text(c, "save -u test/data/");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "save -u test/data/3lines.txt");
    reset_completion(c);

    cmdline_set_text(c, "include -b r");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "include -b rc ");
    reset_completion(c);

    cmdline_set_text(c, "option gitcom");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "option gitcommit ");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "option gitcommit auto-indent");
    reset_completion(c);

    cmdline_set_text(c, "errorfmt c r ");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "errorfmt c r _");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "errorfmt c r column");
    reset_completion(c);

    cmdline_set_text(c, "ft javasc");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "ft javascript ");
    reset_completion(c);

    cmdline_set_text(c, "macro rec");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "macro record ");
    reset_completion(c);

    cmdline_set_text(c, "repeat 3 ");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "repeat 3 alias");
    reset_completion(c);

    cmdline_set_text(c, "repeat 3 hi ");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "repeat 3 hi activetab");
    reset_completion(c);

    cmdline_set_text(c, "left; right; word-");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "left; right; word-bwd");
    reset_completion(c);
}

// This should only be run after init_headless_mode() because the completions
// depend on the buffer and default config being initialized
static void test_complete_command_extra(void)
{
    CommandLine *c = &editor.cmdline;
    cmdline_set_text(c, "show bi");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "show bind ");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "show bind C-?");
    complete_command_prev(c);
    EXPECT_STRING_EQ(c->buf, "show bind up");
    reset_completion(c);

    cmdline_set_text(c, "show errorfmt gc");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "show errorfmt gcc ");
    reset_completion(c);

    cmdline_set_text(c, "option c expand-tab ");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "option c expand-tab true ");
    reset_completion(c);

    const CommandSet *cmds = &normal_commands;
    handle_command(cmds, "run -s mkdir -p $HOME/sub", false);
    handle_command(cmds, "run -s touch $HOME/file $HOME/sub/subfile", false);

    cmdline_set_text(c, "open ~/");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "open ~/file");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "open ~/sub/");
    reset_completion(c);

    cmdline_set_text(c, "open ~/sub/");
    complete_command_next(c);
    EXPECT_STRING_EQ(c->buf, "open ~/sub/subfile ");
    reset_completion(c);
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
