#include "test.h"
#include "../src/command.h"
#include "../src/debug.h"

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
        IEXPECT_GT(strcmp(cmd->name, commands[i - 1].name), 0);
    }
}

void test_command(void)
{
    test_parse_command_arg();
    test_commands_array();
}
