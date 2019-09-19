#include "test.h"
#include "../src/command.h"
#include "../src/debug.h"
#include "../src/parse-args.h"

static void test_parse_command_arg(void)
{
    #define PARSE_ARG_LITERAL(s) parse_command_arg(s, STRLEN(s), false)

    char *arg = PARSE_ARG_LITERAL("");
    EXPECT_STREQ(arg, "");
    free(arg);

    arg = PARSE_ARG_LITERAL("\"\\u148A\"xyz'foo'\"\\x5A\"\\;\tbar");
    EXPECT_STREQ(arg, "\xE1\x92\x8AxyzfooZ;");
    free(arg);

    #undef PARSE_ARG_LITERAL
}

static void test_parse_commands(void)
{
    PointerArray array = PTR_ARRAY_INIT;
    CommandParseError err = 0;
    EXPECT_TRUE(parse_commands(&array, " left  -c;;", &err));
    EXPECT_EQ(array.count, 5);
    EXPECT_STREQ(array.ptrs[0], "left");
    EXPECT_STREQ(array.ptrs[1], "-c");
    EXPECT_NULL(array.ptrs[2]);
    EXPECT_NULL(array.ptrs[3]);
    EXPECT_NULL(array.ptrs[4]);
    EXPECT_EQ(err, 0);
    ptr_array_free(&array);

    EXPECT_TRUE(parse_commands(&array, "save -e UTF-8 file.c; close -q", &err));
    EXPECT_EQ(array.count, 8);
    EXPECT_STREQ(array.ptrs[0], "save");
    EXPECT_STREQ(array.ptrs[1], "-e");
    EXPECT_STREQ(array.ptrs[2], "UTF-8");
    EXPECT_STREQ(array.ptrs[3], "file.c");
    EXPECT_NULL(array.ptrs[4]);
    EXPECT_STREQ(array.ptrs[5], "close");
    EXPECT_STREQ(array.ptrs[6], "-q");
    EXPECT_NULL(array.ptrs[7]);
    EXPECT_EQ(err, 0);
    ptr_array_free(&array);

    EXPECT_TRUE(parse_commands(&array, "\n ; ; \t\n ", &err));
    EXPECT_EQ(array.count, 3);
    EXPECT_NULL(array.ptrs[0]);
    EXPECT_NULL(array.ptrs[1]);
    EXPECT_NULL(array.ptrs[2]);
    EXPECT_EQ(err, 0);
    ptr_array_free(&array);

    EXPECT_TRUE(parse_commands(&array, "", &err));
    EXPECT_EQ(array.count, 1);
    EXPECT_NULL(array.ptrs[0]);
    EXPECT_EQ(err, 0);
    ptr_array_free(&array);

    EXPECT_FALSE(parse_commands(&array, "insert '... ", &err));
    EXPECT_EQ(err, CMDERR_UNCLOSED_SINGLE_QUOTE);
    ptr_array_free(&array);

    err = 0;
    EXPECT_FALSE(parse_commands(&array, "insert \" ", &err));
    EXPECT_EQ(err, CMDERR_UNCLOSED_DOUBLE_QUOTE);
    ptr_array_free(&array);

    err = 0;
    EXPECT_FALSE(parse_commands(&array, "insert \"\\\" ", &err));
    EXPECT_EQ(err, CMDERR_UNCLOSED_DOUBLE_QUOTE);
    ptr_array_free(&array);

    err = 0;
    EXPECT_FALSE(parse_commands(&array, "insert \\", &err));
    EXPECT_EQ(err, CMDERR_UNEXPECTED_EOF);
    ptr_array_free(&array);
}

static void test_parse_args(void)
{
    const char *cmd_str = "open -g file.c file.h *.mk -e UTF-8";
    PointerArray array = PTR_ARRAY_INIT;
    CommandParseError err = 0;
    ASSERT_TRUE(parse_commands(&array, cmd_str, &err));
    ASSERT_EQ(array.count, 8);

    const Command *cmd = find_command(commands, array.ptrs[0]);
    ASSERT_NONNULL(cmd);
    EXPECT_STREQ(cmd->name, "open");

    CommandArgs a = {.args = (char**)array.ptrs + 1};
    ASSERT_TRUE(parse_args(cmd, &a));
    EXPECT_EQ(a.nr_flags, 2);
    EXPECT_EQ(a.flags[0], 'g');
    EXPECT_EQ(a.flags[1], 'e');
    EXPECT_EQ(a.flags[2], '\0');
    EXPECT_EQ(a.nr_args, 3);
    EXPECT_STREQ(a.args[0], "UTF-8");
    EXPECT_STREQ(a.args[1], "file.c");
    EXPECT_STREQ(a.args[2], "file.h");
    EXPECT_STREQ(a.args[3], "*.mk");
    EXPECT_NULL(a.args[4]);

    ptr_array_free(&array);
}

static void test_commands_array(void)
{
    static const size_t name_size = ARRAY_COUNT(commands[0].name);
    static const size_t flags_size = ARRAY_COUNT(commands[0].flags);

    size_t n = 0;
    while (commands[n].cmd) {
        n++;
        BUG_ON(n > 500);
    }

    for (size_t i = 1; i < n; i++) {
        const Command *const cmd = &commands[i];

        // Check that fixed-size arrays are null-terminated within bounds
        ASSERT_EQ(cmd->name[name_size - 1], '\0');
        ASSERT_EQ(cmd->flags[flags_size - 1], '\0');

        // Check that array is sorted by name field, in binary searchable order
        IEXPECT_TRUE(strcmp(cmd->name, commands[i - 1].name) > 0);
    }
}

static void test_command_struct_layout(void)
{
    const Command *cmd = find_command(commands, "filter");
    EXPECT_STREQ(cmd->name, "filter");
    EXPECT_STREQ(cmd->flags, "-");
    EXPECT_EQ(cmd->min_args, 1);
    EXPECT_EQ(cmd->max_args, CMD_ARG_MAX);
    EXPECT_NONNULL(cmd->cmd);
}

DISABLE_WARNING("-Wmissing-prototypes")

void test_command(void)
{
    test_parse_command_arg();
    test_parse_commands();
    test_parse_args();
    test_commands_array();
    test_command_struct_layout();
}
