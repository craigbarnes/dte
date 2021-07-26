#include "test.h"
#include "bind.h"
#include "block.h"
#include "buffer.h"
#include "change.h"
#include "commands.h"
#include "util/str-util.h"

static void test_add_binding(void)
{
    KeyCode key = MOD_CTRL | MOD_SHIFT | KEY_F12;
    const KeyBinding *bind = lookup_binding(INPUT_NORMAL, key);
    EXPECT_NULL(bind);

    const Command *insert_cmd = find_normal_command("insert");
    ASSERT_NONNULL(insert_cmd);

    const char *cmd_str = "insert xyz";
    add_binding(INPUT_NORMAL, key, cmd_str);
    bind = lookup_binding(INPUT_NORMAL, key);
    ASSERT_NONNULL(bind);
    EXPECT_PTREQ(bind->cmd, insert_cmd);
    EXPECT_EQ(bind->a.nr_args, 1);
    EXPECT_EQ(bind->a.nr_flags, 0);
    EXPECT_STREQ(bind->cmd_str, cmd_str);

    remove_binding(INPUT_NORMAL, key);
    EXPECT_NULL(lookup_binding(INPUT_NORMAL, key));
}

static void test_handle_binding(void)
{
    const Command *insert = find_normal_command("insert");
    ASSERT_NONNULL(insert);

    handle_command(&normal_commands, "bind C-S-F11 'insert zzz'; open", false);

    // Bound command should be cached
    KeyCode key = MOD_CTRL | MOD_SHIFT | KEY_F11;
    const KeyBinding *binding = lookup_binding(INPUT_NORMAL, key);
    ASSERT_NONNULL(binding);
    EXPECT_PTREQ(binding->cmd, insert);
    EXPECT_EQ(binding->a.nr_flags, 0);
    EXPECT_EQ(binding->a.nr_args, 1);
    EXPECT_STREQ(binding->a.args[0], "zzz");
    EXPECT_NULL(binding->a.args[1]);

    handle_binding(INPUT_NORMAL, key);
    const Block *block = BLOCK(buffer->blocks.next);
    ASSERT_NONNULL(block);
    ASSERT_EQ(block->size, 4);
    EXPECT_EQ(block->nl, 1);
    EXPECT_MEMEQ(block->data, "zzz\n", 4);

    EXPECT_TRUE(undo());
    EXPECT_EQ(block->size, 0);
    EXPECT_EQ(block->nl, 0);
    EXPECT_FALSE(undo());
    handle_command(&normal_commands, "close", false);
}

static const TestEntry tests[] = {
    TEST(test_add_binding),
    TEST(test_handle_binding),
};

const TestGroup bind_tests = TEST_GROUP(tests);
