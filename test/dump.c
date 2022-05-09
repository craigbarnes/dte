#include "test.h"
#include "bind.h"
#include "command/args.h"
#include "command/parse.h"
#include "commands.h"
#include "compiler.h"
#include "config.h"
#include "editor.h"
#include "filetype.h"
#include "frame.h"
#include "options.h"
#include "show.h"
#include "syntax/color.h"
#include "util/str-util.h"

static const struct {
    const char name[10];
    bool check_parse;
    bool check_name;
    String (*dump)(EditorState *e);
} handlers[] = {
    {"alias", true, true, dump_normal_aliases},
    {"bind", true, true, dump_bindings},
    {"cursor", true, true, dump_cursors},
    {"errorfmt", true, true, dump_compilers},
    {"ft", true, true, do_dump_filetypes},
    {"hi", true, true, do_dump_hl_colors},
    {"include", false, false, do_dump_builtin_configs},
    {"option", true, false, do_dump_options},
    {"wsplit", false, false, dump_frames},
};

static void test_dump_handlers(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    const CommandSet *cmds = &normal_commands;
    void *ud = cmds->userdata;
    ASSERT_NONNULL(ud);

    for (size_t i = 0; i < ARRAYLEN(handlers); i++) {
        String str = handlers[i].dump(e);
        size_t pos = 0;
        while (pos < str.len) {
            bool check_parse = handlers[i].check_parse;
            bool check_name = handlers[i].check_name;
            EXPECT_TRUE(!check_name || check_parse);

            const char *line = buf_next_line(str.buffer, &pos, str.len);
            ASSERT_NONNULL(line);
            if (line[0] == '\0' || line[0] == '#' || !check_parse) {
                continue;
            }

            PointerArray arr = PTR_ARRAY_INIT;
            CommandParseError parse_err = parse_commands(cmds, &arr, line);
            EXPECT_EQ(parse_err, CMDERR_NONE);
            EXPECT_TRUE(arr.count >= 2);
            if (parse_err != CMDERR_NONE || arr.count < 2) {
                continue;
            }

            if (check_name) {
                EXPECT_STREQ(arr.ptrs[0], handlers[i].name);
            }

            const Command *cmd = cmds->lookup(arr.ptrs[0]);
            EXPECT_NONNULL(cmd);
            if (!cmd) {
                continue;
            }

            CommandArgs a = cmdargs_new((char**)arr.ptrs + 1, ud);
            ArgParseError arg_err = do_parse_args(cmd, &a);
            EXPECT_EQ(arg_err, ARGERR_NONE);
            if (arg_err != ARGERR_NONE) {
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
