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
    const char name[9];
    bool check_name;
} handlers[] = {
    {"alias", true},
    {"bind", true},
    {"cursor", true},
    {"def-mode", true},
    {"errorfmt", true},
    {"ft", true},
    {"hi", true},
    {"include", false},
    {"option", false},
    {"set", true},
    {"wsplit", false},
};

static void test_dump_handlers(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    ASSERT_NONNULL(e);
    ASSERT_NONNULL(e->window);
    ASSERT_NONNULL(e->view);
    ASSERT_NONNULL(e->buffer);
    const CommandRunner runner = normal_mode_cmdrunner(e, false);
    const CommandSet *cmds = runner.cmds;
    ASSERT_NONNULL(cmds);
    ASSERT_NONNULL(cmds->lookup);

    for (size_t i = 0; i < ARRAYLEN(handlers); i++) {
        const ShowHandler *handler = lookup_show_handler(handlers[i].name);
        ASSERT_NONNULL(handler);
        ASSERT_NONNULL(handler->dump);
        String str = handler->dump(e);
        for (size_t pos = 0, len = str.len; pos < len; ) {
            bool check_parse = !!(handler->flags & SHOW_DTERC);
            bool check_name = handlers[i].check_name;
            EXPECT_TRUE(!check_name || check_parse);

            const char *line = buf_next_line(str.buffer, &pos, len);
            ASSERT_NONNULL(line);
            if (line[0] == '\0' || line[0] == '#' || !check_parse) {
                continue;
            }

            PointerArray arr = PTR_ARRAY_INIT;
            CommandParseError parse_err = parse_commands(&runner, &arr, line);
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

            CommandArgs a = cmdargs_new((char**)arr.ptrs + 1);
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
