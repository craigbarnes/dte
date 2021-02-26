#include "test.h"
#include "bind.h"
#include "block.h"
#include "buffer.h"
#include "change.h"
#include "commands.h"
#include "util/str-util.h"

static void test_handle_binding(void)
{
    handle_command(&commands, "bind ^A 'insert zzz'; open", false);

    // Bound command should be cached
    const KeyBinding *binding = lookup_binding(MOD_CTRL | 'A');
    const Command *insert = find_normal_command("insert");
    ASSERT_NONNULL(binding);
    ASSERT_NONNULL(insert);
    EXPECT_PTREQ(binding->cmd, insert);
    EXPECT_EQ(binding->a.nr_flags, 0);
    EXPECT_EQ(binding->a.nr_args, 1);
    EXPECT_STREQ(binding->a.args[0], "zzz");
    EXPECT_NULL(binding->a.args[1]);

    handle_binding(MOD_CTRL | 'A');
    const Block *block = BLOCK(buffer->blocks.next);
    ASSERT_NONNULL(block);
    ASSERT_EQ(block->size, 4);
    EXPECT_EQ(block->nl, 1);
    EXPECT_TRUE(mem_equal(block->data, "zzz\n", 4));
    EXPECT_TRUE(undo());
    EXPECT_EQ(block->size, 0);
    EXPECT_EQ(block->nl, 0);
    EXPECT_FALSE(undo());
    handle_command(&commands, "close", false);
}

static const TestEntry tests[] = {
    TEST(test_handle_binding),
};

const TestGroup bind_tests = TEST_GROUP(tests);
