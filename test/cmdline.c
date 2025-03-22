#include <stdlib.h>
#include "test.h"
#include "cmdline.h"
#include "commands.h"
#include "completion.h"
#include "editor.h"
#include "mode.h"

static void test_command_mode(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    CommandLine *c = &e->cmdline;
    push_input_mode(e, e->command_mode);
    EXPECT_PTREQ(e->mode, e->command_mode);

    EXPECT_TRUE(handle_input(e, 'a'));
    EXPECT_EQ(c->pos, 1);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "a");

    EXPECT_TRUE(handle_input(e, 0x1F999));
    EXPECT_EQ(c->pos, 5);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "a\xF0\x9F\xA6\x99");

    CommandRunner runner = cmdrunner(e, &cmd_mode_commands);
    EXPECT_PTREQ(runner.e, e);
    EXPECT_PTREQ(runner.cmds, &cmd_mode_commands);
    EXPECT_NULL(runner.lookup_alias);
    EXPECT_EQ(runner.recursion_count, 0);
    EXPECT_FALSE(runner.allow_recording);
    EXPECT_TRUE(runner.expand_tilde_slash);

    // Delete at end-of-line should do nothing
    CommandRunner *r = &runner;
    EXPECT_TRUE(handle_command(r, "delete"));
    EXPECT_EQ(c->pos, 5);
    EXPECT_EQ(c->buf.len, 5);

    EXPECT_TRUE(handle_command(r, "erase"));
    EXPECT_EQ(c->pos, 1);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "a");

    EXPECT_TRUE(handle_command(r, "erase"));
    EXPECT_EQ(c->pos, 0);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "");

    cmdline_set_text(c, "word1 word2 word3 word4");
    EXPECT_TRUE(handle_command(r, "eol"));
    EXPECT_EQ(c->pos, 23);

    EXPECT_TRUE(handle_command(r, "erase-word"));
    EXPECT_EQ(c->pos, 18);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "word1 word2 word3 ");

    EXPECT_TRUE(handle_command(r, "erase-word"));
    EXPECT_EQ(c->pos, 12);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "word1 word2 ");

    EXPECT_TRUE(handle_command(r, "word-bwd"));
    EXPECT_EQ(c->pos, 6);

    EXPECT_TRUE(handle_command(r, "word-fwd"));
    EXPECT_EQ(c->pos, 12);

    EXPECT_TRUE(handle_command(r, "bol"));
    EXPECT_EQ(c->pos, 0);

    EXPECT_TRUE(handle_command(r, "delete-word"));
    EXPECT_EQ(c->pos, 0);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "word2 ");

    EXPECT_TRUE(handle_command(r, "right"));
    EXPECT_EQ(c->pos, 1);

    EXPECT_TRUE(handle_command(r, "erase-bol"));
    EXPECT_EQ(c->pos, 0);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "ord2 ");

    EXPECT_TRUE(handle_command(r, "delete"));
    EXPECT_EQ(c->pos, 0);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "rd2 ");

    EXPECT_TRUE(handle_command(r, "delete-eol"));
    EXPECT_EQ(c->pos, 0);
    EXPECT_EQ(c->buf.len, 0);

    // Left at beginning-of-line should do nothing
    EXPECT_TRUE(handle_command(r, "left"));
    EXPECT_EQ(c->pos, 0);

    cmdline_set_text(c, "...");
    EXPECT_EQ(c->pos, 3);
    EXPECT_EQ(c->buf.len, 3);

    EXPECT_TRUE(handle_command(r, "bol"));
    EXPECT_EQ(c->pos, 0);
    EXPECT_EQ(c->buf.len, 3);

    EXPECT_TRUE(handle_command(r, "right"));
    EXPECT_EQ(c->pos, 1);

    EXPECT_TRUE(handle_command(r, "word-bwd"));
    EXPECT_EQ(c->pos, 0);

    EXPECT_TRUE(handle_command(r, "eol"));
    EXPECT_EQ(c->pos, 3);

    EXPECT_TRUE(handle_command(r, "cancel"));
    EXPECT_EQ(c->pos, 0);
    EXPECT_NULL(c->search_pos);
    EXPECT_EQ(c->buf.len, 0);
    EXPECT_PTREQ(e->mode, e->normal_mode);

    string_free(&c->buf);
    EXPECT_NULL(c->buf.buffer);
    EXPECT_EQ(c->buf.alloc, 0);
}

#define ENV_VAR_PREFIX "D__p_tYmz3_"
#define ENV_VAR_NAME ENV_VAR_PREFIX "_VAR"

static void test_complete_command(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    CommandLine *c = &e->cmdline;
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "alias");
    maybe_reset_completion(c);

    cmdline_set_text(c, "bind C-invalidkey");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "bind C-invalidkey");
    reset_completion(c);

    cmdline_set_text(c, "bind -s C-M-S-l");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "bind -s C-M-S-l");
    reset_completion(c);

    cmdline_set_text(c, "bind -cs ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "bind -cs ");
    reset_completion(c);

    cmdline_set_text(c, "bind -c -T search ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "bind -c -T search ");
    reset_completion(c);

    cmdline_set_text(c, "bind -T ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "bind -T command");
    reset_completion(c);

    cmdline_set_text(c, "bind -T se");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "bind -T search ");
    reset_completion(c);

    cmdline_set_text(c, "def-mode ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "def-mode ");
    reset_completion(c);

    cmdline_set_text(c, "def-mode xyz ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "def-mode xyz normal ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "def-mode xyz normal ");
    reset_completion(c);

    cmdline_set_text(c, "mode ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "mode command");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "mode normal");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "mode search");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "mode command");
    reset_completion(c);

    cmdline_set_text(c, "wrap");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "wrap-paragraph ");
    reset_completion(c);

    cmdline_set_text(c, "open test/data/.e");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "open test/data/.editorconfig ");
    reset_completion(c);

    cmdline_set_text(c, "open GNUma");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "open GNUmakefile ");
    reset_completion(c);

    cmdline_set_text(c, "wsplit -bhr test/data/.e");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "wsplit -bhr test/data/.editorconfig ");
    reset_completion(c);

    cmdline_set_text(c, "toggle ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "toggle auto-indent");
    reset_completion(c);

    cmdline_set_text(c, "set expand-tab f");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "set expand-tab false ");
    reset_completion(c);

    cmdline_set_text(c, "set case-sensitive-search a");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "set case-sensitive-search auto ");
    reset_completion(c);

    ASSERT_EQ(setenv(ENV_VAR_NAME, "xyz", 1), 0);

    cmdline_set_text(c, "insert $" ENV_VAR_PREFIX);
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "insert $" ENV_VAR_NAME);
    reset_completion(c);

    cmdline_set_text(c, "setenv " ENV_VAR_PREFIX);
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "setenv " ENV_VAR_NAME " ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "setenv " ENV_VAR_NAME " xyz ");
    reset_completion(c);

    ASSERT_EQ(unsetenv(ENV_VAR_NAME), 0);

    cmdline_clear(c);
    complete_command_prev(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "alias");
    complete_command_prev(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "wswap");
    complete_command_prev(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "wsplit");
    reset_completion(c);

    cmdline_set_text(c, "hi default ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "hi default black");
    complete_command_prev(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "hi default yellow");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "hi default black");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "hi default blink");
    reset_completion(c);

    cmdline_set_text(c, "cursor i");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "cursor insert ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "cursor insert bar");
    reset_completion(c);

    cmdline_set_text(c, "cursor default blo");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "cursor default block ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "cursor default block #22AABB");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "cursor default block default");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "cursor default block keep");
    reset_completion(c);

    cmdline_set_text(c, "show opt");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "show option ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "show option auto-indent");
    reset_completion(c);

    cmdline_set_text(c, "show builtin syntax/gitcom");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "show builtin syntax/gitcommit ");
    reset_completion(c);

    cmdline_set_text(c, "set ws-error tr");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "set ws-error trailing ");
    reset_completion(c);

    cmdline_set_text(c, "set ws-error special,tab-");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "set ws-error special,tab-after-indent");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "set ws-error special,tab-indent");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "set ws-error special,tab-after-indent");
    reset_completion(c);

    cmdline_set_text(c, "set esc-timeout ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "set esc-timeout 100 ");
    reset_completion(c);

    cmdline_set_text(c, "set x y tab-b");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "set x y tab-bar ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "set x y tab-bar true ");
    reset_completion(c);

    cmdline_set_text(c, "save -u test/data/");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "save -u test/data/3lines.txt");
    reset_completion(c);

    cmdline_set_text(c, "include -b r");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "include -b rc ");
    reset_completion(c);

    cmdline_set_text(c, "option gitcom");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "option gitcommit ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "option gitcommit auto-indent");
    reset_completion(c);

    cmdline_set_text(c, "errorfmt c r ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "errorfmt c r _");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "errorfmt c r column");
    reset_completion(c);

    cmdline_set_text(c, "ft javasc");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "ft javascript ");
    reset_completion(c);

    cmdline_set_text(c, "macro rec");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "macro record ");
    reset_completion(c);

    cmdline_set_text(c, "move-tab ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "move-tab left");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "move-tab right");
    reset_completion(c);

    cmdline_set_text(c, "quit ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "quit 0");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "quit 1");
    reset_completion(c);

    cmdline_set_text(c, "repeat 3 ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "repeat 3 alias");
    reset_completion(c);

    cmdline_set_text(c, "repeat 3 hi ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "repeat 3 hi activetab");
    reset_completion(c);

    cmdline_set_text(c, "left; right; word-");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "left; right; word-bwd");
    reset_completion(c);

    cmdline_set_text(c, "replace -");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "replace -b");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "replace -c");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "replace -e");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "replace -g");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "replace -i");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "replace -b");
    reset_completion(c);

    cmdline_set_text(c, "replace -- -");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "replace -- -");
    reset_completion(c);

    cmdline_set_text(c, "left -c; replace -b -");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "left -c; replace -b -c");
    reset_completion(c);

    cmdline_set_text(c, "replace -bcei -");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "replace -bcei -g ");
    reset_completion(c);

    cmdline_set_text(c, "replace -bcegi -");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "replace -bcegi -");
    reset_completion(c);

    cmdline_set_text(c, "replace -i");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "replace -i ");
    reset_completion(c);

    cmdline_set_text(c, "wswap -");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "wswap -");
    reset_completion(c);

    cmdline_set_text(c, "exec -");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "exec -e");
    reset_completion(c);

    cmdline_set_text(c, "exec -o");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "exec -o ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "exec -o buffer");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "exec -o echo");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "exec -o eval");
    reset_completion(c);

    cmdline_set_text(c, "exec -- -");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "exec -- -");
    reset_completion(c);

    cmdline_set_text(c, "exec -p ls -");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "exec -p ls -");
    reset_completion(c);

    cmdline_set_text(c, "exec -p ls -l");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "exec -p ls -l");
    reset_completion(c);

    cmdline_set_text(c, "exec -s -i buffer -o buffer -");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "exec -s -i buffer -o buffer -e");
    reset_completion(c);

    cmdline_set_text(c, "exec -s -i buffer ls -");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "exec -s -i buffer ls -");
    reset_completion(c);

    cmdline_set_text(c, "replace -cg");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "replace -cg");
    reset_completion(c);

    cmdline_set_text(c, "left -");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "left -c");
    reset_completion(c);

    cmdline_set_text(c, "option -");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "option -r ");
    reset_completion(c);

    cmdline_set_text(c, "quit 1 -");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "quit 1 -C");
    reset_completion(c);

    cmdline_set_text(c, "quit 1 2 -");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "quit 1 2 -");
    reset_completion(c);

    const Command *wflip = find_normal_command("wflip");
    EXPECT_EQ(wflip->flags[0], '\0');
    EXPECT_EQ(wflip->max_args, 0);

    cmdline_set_text(c, "wflip ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "wflip ");
    reset_completion(c);

    cmdline_set_text(c, "wflip -");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "wflip -");
    reset_completion(c);

    cmdline_set_text(c, "wflip -- -");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "wflip -- -");
    reset_completion(c);
}

// This should only be run after init_headless_mode() because the completions
// depend on the buffer and default config being initialized
static void test_complete_command_extra(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    CommandLine *c = &e->cmdline;

    cmdline_set_text(c, "alias reverse-li");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "alias reverse-lines ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "alias reverse-lines 'filter tac' ");
    reset_completion(c);

    cmdline_set_text(c, "bind C-z");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "bind C-z ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "bind C-z undo ");
    reset_completion(c);

    cmdline_set_text(c, "bind -s C-g");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "bind -s C-g ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "bind -s C-g cancel ");
    reset_completion(c);

    cmdline_set_text(c, "bind -T command tab ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "bind -T command tab complete-next ");
    reset_completion(c);

    cmdline_set_text(c, "bind -qc C-left ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "bind -qc C-left word-bwd ");
    reset_completion(c);

    cmdline_set_text(c, "compile -1s bas");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "compile -1s basic ");
    reset_completion(c);

    cmdline_set_text(c, "errorfmt bas");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "errorfmt basic ");
    reset_completion(c);

    cmdline_set_text(c, "errorfmt xyz '^(.*)$' m");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "errorfmt xyz '^(.*)$' message ");
    reset_completion(c);

    cmdline_set_text(c, "show bi");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "show bind ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "show bind C-M-S-delete");
    complete_command_prev(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "show bind up");
    reset_completion(c);

    cmdline_set_text(c, "show errorfmt gc");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "show errorfmt gcc ");
    reset_completion(c);

    cmdline_set_text(c, "option c expand-tab ");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "option c expand-tab true ");
    reset_completion(c);

    static const char create_files[] =
        "run -s mkdir -p $HOME/sub;"
        "run -s touch $HOME/file $HOME/sub/subfile;";

    EXPECT_TRUE(handle_normal_command(e, create_files, false));
    cmdline_set_text(c, "open ~/");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "open ~/file");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "open ~/sub/");
    reset_completion(c);

    cmdline_set_text(c, "open ~/sub/");
    complete_command_next(e);
    EXPECT_STRING_EQ_CSTRING(&c->buf, "open ~/sub/subfile ");
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
