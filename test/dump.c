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

typedef enum {
    CHECK_NAME = 1u << 0, // Non-blank lines must begin with `name`
    CHECK_PARSE = 1u << 1, // Non-blank lines must be parseable by parse_commands()
    ALLOW_EMPTY = 1u << 2, // Dumped string may be empty
} DumpCheckFlags;

static const struct {
    const char name[9];
    uint8_t flags; // DumpCheckFlags
} handlers[] = {
    {"alias", CHECK_NAME | CHECK_PARSE},
    {"bind", CHECK_NAME | CHECK_PARSE},
    {"buffer", 0},
    {"builtin", 0}, // Same as `include`
    {"color", CHECK_PARSE}, // Same as `hi`
    {"command", ALLOW_EMPTY},
    {"cursor", CHECK_NAME | CHECK_PARSE},
    {"def-mode", CHECK_NAME | CHECK_PARSE},
    {"env", 0}, // Similar to `setenv`
    {"errorfmt", CHECK_NAME | CHECK_PARSE},
    {"ft", CHECK_NAME | CHECK_PARSE},
    {"hi", CHECK_NAME | CHECK_PARSE},
    {"include", 0},
    {"macro", 0},
    {"msg", ALLOW_EMPTY},
    {"open", 0},
    {"option", CHECK_PARSE},
    {"search", ALLOW_EMPTY},
    {"set", CHECK_NAME | CHECK_PARSE},
    {"setenv", CHECK_NAME | CHECK_PARSE},
    // {"tag", 0}, // Depends on filesystem state not controlled by the test runner
    {"wsplit", 0},
};

static void test_dump_handlers(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    ASSERT_NONNULL(e);
    ASSERT_NONNULL(e->window);
    ASSERT_NONNULL(e->view);
    ASSERT_NONNULL(e->buffer);
    const CommandRunner runner = normal_mode_cmdrunner(e);
    const CommandSet *cmds = runner.cmds;
    ASSERT_NONNULL(cmds);
    ASSERT_NONNULL(cmds->lookup);
    clear_all_messages(e); // Clear messages for previous `tag` tests

    for (size_t i = 0; i < ARRAYLEN(handlers); i++) {
        bool check_parse = (handlers[i].flags & CHECK_PARSE);
        bool check_name = (handlers[i].flags & CHECK_NAME);
        bool allow_empty_str = (handlers[i].flags & ALLOW_EMPTY);
        EXPECT_TRUE(!check_name || check_parse);

        const char *name = handlers[i].name;
        DumpFunc dump = get_dump_function(name);
        EXPECT_NONNULL(dump);
        if (!dump) {
            continue;
        }

        String str = dump(e);
        if (str.len == 0) {
            if (!allow_empty_str) {
                TEST_FAIL("'show %s' handler returned an empty String", name);
            } else {
                test_pass(ctx);
            }
            EXPECT_EQ(str.alloc, 0);
            EXPECT_NULL(str.buffer);
            string_free(&str);
            continue;
        }

        if (allow_empty_str) {
            TEST_FAIL (
                "ALLOW_EMPTY flag set for '%s', but dump() produced:  %s",
                name, string_borrow_cstring(&str)
            );
        } else {
            test_pass(ctx);
        }

        // The last line of generated text must end with a newline
        // (see comment in get_delim_str())
        ASSERT_EQ(str.buffer[str.len - 1], '\n');

        for (size_t pos = 0, len = str.len; pos < len; ) {
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
                EXPECT_STREQ(arr.ptrs[0], name);
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
