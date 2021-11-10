#include "test.h"
#include "bind.h"
#include "block.h"
#include "buffer.h"
#include "change.h"
#include "command/args.h"
#include "commands.h"
#include "editor.h"
#include "util/str-util.h"

static void test_add_binding(void)
{
    KeyBindingGroup *kbg = &editor.bindings[INPUT_NORMAL];
    KeyCode key = MOD_CTRL | MOD_SHIFT | KEY_F12;
    const CachedCommand *bind = lookup_binding(kbg, key);
    EXPECT_NULL(bind);

    const Command *insert_cmd = find_normal_command("insert");
    ASSERT_NONNULL(insert_cmd);

    const char *cmd_str = "insert xyz";
    add_binding(kbg, key, cmd_str);
    bind = lookup_binding(kbg, key);
    ASSERT_NONNULL(bind);
    EXPECT_PTREQ(bind->cmd, insert_cmd);
    EXPECT_EQ(bind->a.nr_args, 1);
    EXPECT_EQ(bind->a.nr_flags, 0);
    EXPECT_STREQ(bind->cmd_str, cmd_str);

    remove_binding(kbg, key);
    EXPECT_NULL(lookup_binding(kbg, key));
}

static void test_handle_binding(void)
{
    const Command *insert = find_normal_command("insert");
    ASSERT_NONNULL(insert);

    const CommandSet *cmds = &normal_commands;
    handle_command(cmds, "open; bind C-S-F11 'insert -m zzz'", false);

    // Bound command should be cached
    KeyBindingGroup *kbg = &editor.bindings[INPUT_NORMAL];
    KeyCode key = MOD_CTRL | MOD_SHIFT | KEY_F11;
    const CachedCommand *binding = lookup_binding(kbg, key);
    ASSERT_NONNULL(binding);
    EXPECT_PTREQ(binding->cmd, insert);
    EXPECT_EQ(binding->a.nr_flags, 1);
    EXPECT_EQ(binding->a.nr_args, 1);
    EXPECT_STREQ(binding->a.args[0], "zzz");
    EXPECT_NULL(binding->a.args[1]);
    EXPECT_EQ(binding->a.flags[0], 'm');
    EXPECT_EQ(binding->a.flags[1], '\0');
    EXPECT_TRUE(cmdargs_has_flag(&binding->a, 'm'));

    handle_binding(kbg, key);
    const Block *block = BLOCK(editor.buffer->blocks.next);
    ASSERT_NONNULL(block);
    ASSERT_EQ(block->size, 4);
    EXPECT_EQ(block->nl, 1);
    EXPECT_MEMEQ(block->data, "zzz\n", 4);

    handle_binding(kbg, MOD_CTRL | '?');
    ASSERT_EQ(block->size, 3);
    EXPECT_EQ(block->nl, 1);
    EXPECT_MEMEQ(block->data, "zz\n", 3);

    handle_binding(kbg, MOD_CTRL | MOD_META | '?');
    ASSERT_EQ(block->size, 1);
    EXPECT_EQ(block->nl, 1);
    EXPECT_MEMEQ(block->data, "\n", 1);

    View *view = editor.view;
    ASSERT_NONNULL(view);
    EXPECT_TRUE(undo(view));
    ASSERT_EQ(block->size, 3);
    EXPECT_TRUE(undo(view));
    ASSERT_EQ(block->size, 4);
    EXPECT_TRUE(undo(view));
    EXPECT_EQ(block->size, 0);
    EXPECT_EQ(block->nl, 0);
    EXPECT_FALSE(undo(view));
    handle_command(cmds, "close", false);
}

static const TestEntry tests[] = {
    TEST(test_add_binding),
    TEST(test_handle_binding),
};

const TestGroup bind_tests = TEST_GROUP(tests);
