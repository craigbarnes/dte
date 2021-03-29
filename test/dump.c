#include "test.h"
#include "alias.h"
#include "bind.h"
#include "command/args.h"
#include "command/parse.h"
#include "commands.h"
#include "compiler.h"
#include "config.h"
#include "filetype.h"
#include "frame.h"
#include "options.h"
#include "syntax/color.h"
#include "util/str-util.h"

static const struct {
    const char name[10];
    bool check_parse;
    bool check_name;
    String (*dump)(void);
} handlers[] = {
    {"alias", true, true, dump_aliases},
    {"bind", true, true, dump_bindings},
    {"errorfmt", true, true, dump_compilers},
    {"ft", true, true, dump_ft},
    {"hi", true, true, dump_hl_colors},
    {"include", false, false, dump_builtin_configs},
    {"option", true, false, dump_options},
    {"wsplit", false, false, dump_frames},
};

static void test_dump_handlers(void)
{
    for (size_t i = 0; i < ARRAY_COUNT(handlers); i++) {
        String str = handlers[i].dump();
        size_t pos = 0;
        while (pos < str.len) {
            const char *line = buf_next_line(str.buffer, &pos, str.len);
            ASSERT_NONNULL(line);
            char c = line[0];
            if (c == '\0' || c == '#' || !handlers[i].check_parse) {
                continue;
            }

            PointerArray arr = PTR_ARRAY_INIT;
            CommandParseError parse_err = parse_commands(&arr, line);
            EXPECT_EQ(parse_err, CMDERR_NONE);
            EXPECT_TRUE(arr.count >= 2);
            if (parse_err != CMDERR_NONE || arr.count < 2) {
                continue;
            }

            if (handlers[i].check_name) {
                EXPECT_STREQ(arr.ptrs[0], handlers[i].name);
            }

            const Command *cmd = find_normal_command(arr.ptrs[0]);
            EXPECT_NONNULL(cmd);
            if (!cmd) {
                continue;
            }

            CommandArgs a = {.args = (char**)arr.ptrs + 1};
            ArgParseError arg_err = do_parse_args(cmd, &a);
            EXPECT_EQ(arg_err, 0);
            if (arg_err != 0) {
                continue;
            }
            ptr_array_free(&arr);
        }
        string_free(&str);
    }
}

static const TestEntry tests[] = {
    TEST(test_dump_handlers),
};

const TestGroup dump_tests = TEST_GROUP(tests);
