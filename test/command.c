#include <limits.h>
#include "test.h"
#include "command/alias.h"
#include "command/args.h"
#include "command/cache.h"
#include "command/parse.h"
#include "command/run.h"
#include "command/serialize.h"
#include "commands.h"
#include "editor.h"
#include "replace.h"
#include "search.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/path.h"
#include "util/str-util.h"

static void test_parse_command_arg(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    const CommandRunner runner = cmdrunner_for_mode(e, INPUT_NORMAL, false);
    ASSERT_PTREQ(runner.userdata, e);
    ASSERT_PTREQ(runner.cmds, &normal_commands);
    EXPECT_PTREQ(runner.lookup_alias, find_normal_alias);
    EXPECT_PTREQ(runner.home_dir, &e->home_dir);
    EXPECT_FALSE(runner.allow_recording);
    EXPECT_EQ(runner.recursion_count, 0);

    // Single, unquoted argument
    char *arg = parse_command_arg(&runner, STRN("arg"), false);
    EXPECT_STREQ(arg, "arg");
    free(arg);

    // Two unquoted, space-separated arguments
    arg = parse_command_arg(&runner, STRN("hello world"), false);
    EXPECT_STREQ(arg, "hello");
    free(arg);

    // Two unquoted, tab-separated arguments
    arg = parse_command_arg(&runner, STRN("hello\tworld"), false);
    EXPECT_STREQ(arg, "hello");
    free(arg);

    // Unquoted argument preceded by whitespace
    arg = parse_command_arg(&runner, STRN(" x"), false);
    EXPECT_STREQ(arg, "");
    free(arg);

    // Single-quoted argument, including whitespace
    arg = parse_command_arg(&runner, STRN("'  foo ' .."), false);
    EXPECT_STREQ(arg, "  foo ");
    free(arg);

    // Several adjacent, quoted strings forming a single argument
    arg = parse_command_arg(&runner, STRN("\"foo\"'bar'' baz '\"etc\"."), false);
    EXPECT_STREQ(arg, "foobar baz etc.");
    free(arg);

    // Control character escapes in a double-quoted string
    arg = parse_command_arg(&runner, STRN("\"\\a\\b\\t\\n\\v\\f\\r\""), false);
    EXPECT_STREQ(arg, "\a\b\t\n\v\f\r");
    free(arg);

    // Backslash escape sequence in a double-quoted string
    arg = parse_command_arg(&runner, STRN("\"\\\\\""), false);
    EXPECT_STREQ(arg, "\\");
    free(arg);

    // Double-quote escape sequence in a double-quoted string
    arg = parse_command_arg(&runner, STRN("\"\\\"\""), false);
    EXPECT_STREQ(arg, "\"");
    free(arg);

    // Escape character escape sequence in a double-quoted string
    arg = parse_command_arg(&runner, STRN("\"\\e[0m\""), false);
    EXPECT_STREQ(arg, "\033[0m");
    free(arg);

    // Unrecognized escape sequence in a double-quoted string
    arg = parse_command_arg(&runner, STRN("\"\\z\""), false);
    EXPECT_STREQ(arg, "\\z");
    free(arg);

    // Hexadecimal escape sequences in a double-quoted string
    arg = parse_command_arg(&runner, STRN("\"\\x1B[31m\\x7E\\x2f\\x1b[0m\""), false);
    EXPECT_STREQ(arg, "\x1B[31m~/\x1B[0m");
    free(arg);

    // Invalid hexadecimal escape sequences
    arg = parse_command_arg(&runner, STRN("\"\\x\\x1\\xFG\\xz1\""), false);
    EXPECT_STREQ(arg, "Gz1");
    free(arg);

    // Incomplete hexadecimal escape sequence
    arg = parse_command_arg(&runner, STRN("\"\\x"), false);
    EXPECT_STREQ(arg, "");
    free(arg);

    // 4-digit Unicode escape sequence
    arg = parse_command_arg(&runner, STRN("\"\\u148A\""), false);
    EXPECT_STREQ(arg, "\xE1\x92\x8A");
    free(arg);

    // 8-digit Unicode escape sequence
    arg = parse_command_arg(&runner, STRN("\"\\U0001F4A4\""), false);
    EXPECT_STREQ(arg, "\xF0\x9F\x92\xA4");
    free(arg);

    // "\U" escape sequence terminated by non-hexadecimal character
    arg = parse_command_arg(&runner, STRN("\"\\U1F4A4...\""), false);
    EXPECT_STREQ(arg, "\xF0\x9F\x92\xA4...");
    free(arg);

    // Incomplete Unicode escape sequence
    arg = parse_command_arg(&runner, STRN("\"\\u"), false);
    EXPECT_STREQ(arg, "");
    free(arg);

    // Invalid Unicode escape sequence
    arg = parse_command_arg(&runner, STRN("\"\\ugef\""), false);
    EXPECT_STREQ(arg, "gef");
    free(arg);

    // Unsupported, escape-like sequences in a single-quoted string
    arg = parse_command_arg(&runner, STRN("'\\t\\n'"), false);
    EXPECT_STREQ(arg, "\\t\\n");
    free(arg);

    // Trailing backslash
    // Note: `s` is unterminated, to allow ASan to catch OOB reads
    static const NONSTRING char s[4] = "123\\";
    arg = parse_command_arg(&runner, s, sizeof s, false);
    EXPECT_STREQ(arg, "123");
    free(arg);

    // Single-quoted, empty string
    arg = parse_command_arg(&runner, STRN("''"), false);
    EXPECT_STREQ(arg, "");
    free(arg);

    // Double-quoted, empty string
    arg = parse_command_arg(&runner, STRN("\"\""), false);
    EXPECT_STREQ(arg, "");
    free(arg);

    // NULL input with zero length
    arg = parse_command_arg(&runner, NULL, 0, false);
    EXPECT_STREQ(arg, "");
    free(arg);

    // Empty input
    arg = parse_command_arg(&runner, "", 1, false);
    EXPECT_STREQ(arg, "");
    free(arg);

    // Built-in vars (expand to nothing; buffer isn't initialized yet)
    arg = parse_command_arg(&runner, STRN("$FILE' '$FILEDIR' '$FILETYPE' '$LINENO' '$WORD"), false);
    EXPECT_STREQ(arg, "    ");
    free(arg);

    // Built-in $DTE_HOME var (expands to user config dir)
    arg = parse_command_arg(&runner, STRN("$DTE_HOME"), false);
    EXPECT_TRUE(path_is_absolute(arg));
    EXPECT_TRUE(str_has_suffix(arg, "/test/DTE_HOME"));
    free(arg);

    // Environment var (via getenv(3))
    arg = parse_command_arg(&runner, STRN("$DTE_VERSION"), false);
    EXPECT_STREQ(arg, e->version);
    free(arg);

    // Tilde expansion
    arg = parse_command_arg(&runner, STRN("~/filename"), true);
    EXPECT_TRUE(str_has_suffix(arg, "/build/test/HOME/filename"));
    free(arg);

    arg = parse_command_arg(&runner, STRN("'xyz"), false);
    EXPECT_STREQ(arg, "xyz");
    free(arg);

    arg = parse_command_arg(&runner, STRN("\"\\u148A\"xyz'foo'\"\\x5A\"\\;\t."), false);
    EXPECT_STREQ(arg, "\xE1\x92\x8AxyzfooZ;");
    free(arg);
}

static void test_parse_commands(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    const CommandRunner runner = cmdrunner_for_mode(e, INPUT_NORMAL, false);
    PointerArray array = PTR_ARRAY_INIT;
    EXPECT_EQ(parse_commands(&runner, &array, " left  -c;;"), CMDERR_NONE);
    ASSERT_EQ(array.count, 5);
    EXPECT_STREQ(array.ptrs[0], "left");
    EXPECT_STREQ(array.ptrs[1], "-c");
    EXPECT_NULL(array.ptrs[2]);
    EXPECT_NULL(array.ptrs[3]);
    EXPECT_NULL(array.ptrs[4]);
    ptr_array_free(&array);

    EXPECT_EQ(parse_commands(&runner, &array, "save -e UTF-8 file.c; close -q"), CMDERR_NONE);
    ASSERT_EQ(array.count, 8);
    EXPECT_STREQ(array.ptrs[0], "save");
    EXPECT_STREQ(array.ptrs[1], "-e");
    EXPECT_STREQ(array.ptrs[2], "UTF-8");
    EXPECT_STREQ(array.ptrs[3], "file.c");
    EXPECT_NULL(array.ptrs[4]);
    EXPECT_STREQ(array.ptrs[5], "close");
    EXPECT_STREQ(array.ptrs[6], "-q");
    EXPECT_NULL(array.ptrs[7]);
    ptr_array_free(&array);

    EXPECT_EQ(parse_commands(&runner, &array, "\n ; ; \t\n "), CMDERR_NONE);
    ASSERT_EQ(array.count, 3);
    EXPECT_NULL(array.ptrs[0]);
    EXPECT_NULL(array.ptrs[1]);
    EXPECT_NULL(array.ptrs[2]);
    ptr_array_free(&array);

    EXPECT_EQ(parse_commands(&runner, &array, ""), CMDERR_NONE);
    ASSERT_EQ(array.count, 1);
    EXPECT_NULL(array.ptrs[0]);
    ptr_array_free(&array);

    EXPECT_EQ(parse_commands(&runner, &array, "insert '... "), CMDERR_UNCLOSED_SQUOTE);
    ptr_array_free(&array);

    EXPECT_EQ(parse_commands(&runner, &array, "insert \" "), CMDERR_UNCLOSED_DQUOTE);
    ptr_array_free(&array);

    EXPECT_EQ(parse_commands(&runner, &array, "insert \"\\\" "), CMDERR_UNCLOSED_DQUOTE);
    ptr_array_free(&array);

    EXPECT_EQ(parse_commands(&runner, &array, "insert \\"), CMDERR_UNEXPECTED_EOF);
    ptr_array_free(&array);

    EXPECT_EQ(parse_commands(&runner, &array, "insert \"\\"), CMDERR_UNEXPECTED_EOF);
    ptr_array_free(&array);
}

static void test_command_parse_error_to_string(TestContext *ctx)
{
    const char *str = command_parse_error_to_string(CMDERR_UNCLOSED_SQUOTE);
    EXPECT_STREQ(str, "unclosed '");
    str = command_parse_error_to_string(CMDERR_UNCLOSED_DQUOTE);
    EXPECT_STREQ(str, "unclosed \"");
    str = command_parse_error_to_string(CMDERR_UNEXPECTED_EOF);
    EXPECT_STREQ(str, "unexpected EOF");
}

static void test_find_normal_command(TestContext *ctx)
{
    const Command *cmd = find_normal_command("alias");
    ASSERT_NONNULL(cmd);
    EXPECT_STREQ(cmd->name, "alias");

    cmd = find_normal_command("bind");
    ASSERT_NONNULL(cmd);
    EXPECT_STREQ(cmd->name, "bind");

    cmd = find_normal_command("wswap");
    ASSERT_NONNULL(cmd);
    EXPECT_STREQ(cmd->name, "wswap");

    EXPECT_NULL(find_normal_command("alia"));
    EXPECT_NULL(find_normal_command("aliass"));
    EXPECT_NULL(find_normal_command("Alias"));
    EXPECT_NULL(find_normal_command("bin "));
    EXPECT_NULL(find_normal_command("bind "));
    EXPECT_NULL(find_normal_command(" bind"));
    EXPECT_NULL(find_normal_command("bind!"));
    EXPECT_NULL(find_normal_command("bind\n"));
}

static void test_parse_args(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    const CommandRunner runner = cmdrunner_for_mode(e, INPUT_NORMAL, false);
    const CommandSet *cmds = runner.cmds;
    const char *cmd_str = "open -g file.c file.h *.mk -e UTF-8";
    PointerArray array = PTR_ARRAY_INIT;
    ASSERT_NONNULL(cmds);
    ASSERT_EQ(parse_commands(&runner, &array, cmd_str), CMDERR_NONE);
    ASSERT_EQ(array.count, 8);

    const Command *cmd = cmds->lookup(array.ptrs[0]);
    ASSERT_NONNULL(cmd);
    EXPECT_STREQ(cmd->name, "open");

    CommandArgs a = cmdargs_new((char**)array.ptrs + 1);
    ASSERT_EQ(do_parse_args(cmd, &a), ARGERR_NONE);
    EXPECT_EQ(a.nr_flags, 2);
    EXPECT_EQ(a.flags[0], 'g');
    EXPECT_EQ(a.flags[1], 'e');
    EXPECT_EQ(a.flags[2], '\0');
    EXPECT_TRUE(cmdargs_has_flag(&a, 'g'));
    EXPECT_TRUE(cmdargs_has_flag(&a, 'e'));
    EXPECT_FALSE(cmdargs_has_flag(&a, 'f'));
    EXPECT_FALSE(cmdargs_has_flag(&a, 'E'));
    EXPECT_FALSE(cmdargs_has_flag(&a, '0'));
    EXPECT_EQ(a.nr_flag_args, 1);
    EXPECT_STREQ(a.args[0], "UTF-8");
    EXPECT_EQ(a.nr_args, 3);
    EXPECT_STREQ(a.args[1], "file.c");
    EXPECT_STREQ(a.args[2], "file.h");
    EXPECT_STREQ(a.args[3], "*.mk");
    EXPECT_NULL(a.args[4]);

    ptr_array_free(&array);
    EXPECT_NULL(array.ptrs);
    EXPECT_EQ(array.alloc, 0);
    EXPECT_EQ(array.count, 0);

    cmd_str = "bind 1 2 3 4 5 -6";
    ASSERT_EQ(parse_commands(&runner, &array, cmd_str), CMDERR_NONE);
    ASSERT_EQ(array.count, 8);
    EXPECT_STREQ(array.ptrs[0], "bind");
    EXPECT_STREQ(array.ptrs[1], "1");
    EXPECT_STREQ(array.ptrs[6], "-6");
    EXPECT_NULL(array.ptrs[7]);

    cmd = cmds->lookup(array.ptrs[0]);
    ASSERT_NONNULL(cmd);
    EXPECT_STREQ(cmd->name, "bind");
    EXPECT_EQ(cmd->max_args, 2);
    EXPECT_EQ(cmd->flags[0], '-');

    a = cmdargs_new((char**)array.ptrs + 1);
    a.flags[0] = 'X';
    ASSERT_EQ(do_parse_args(cmd, &a), ARGERR_TOO_MANY_ARGUMENTS);
    EXPECT_EQ(a.nr_args, 6);
    EXPECT_EQ(a.nr_flags, 0);
    EXPECT_EQ(a.nr_flag_args, 0);
    EXPECT_EQ(a.flags[0], '\0');
    EXPECT_EQ(a.flag_set, 0);
    ptr_array_free(&array);

    cmd_str = "open \"-\\xff\"";
    ASSERT_EQ(parse_commands(&runner, &array, cmd_str), CMDERR_NONE);
    ASSERT_EQ(array.count, 3);
    EXPECT_STREQ(array.ptrs[0], "open");
    EXPECT_STREQ(array.ptrs[1], "-\xff");
    EXPECT_NULL(array.ptrs[2]);
    cmd = cmds->lookup(array.ptrs[0]);
    ASSERT_NONNULL(cmd);
    a = cmdargs_new((char**)array.ptrs + 1);
    EXPECT_EQ(do_parse_args(cmd, &a), ARGERR_INVALID_OPTION);
    EXPECT_EQ(a.flags[0], '\xff');
    ptr_array_free(&array);

    ASSERT_EQ(parse_commands(&runner, &array, "save -e"), CMDERR_NONE);
    ASSERT_EQ(array.count, 3);
    cmd = cmds->lookup(array.ptrs[0]);
    ASSERT_NONNULL(cmd);
    a = cmdargs_new((char**)array.ptrs + 1);
    EXPECT_EQ(do_parse_args(cmd, &a), ARGERR_OPTION_ARGUMENT_MISSING);
    EXPECT_EQ(a.flags[0], 'e');
    ptr_array_free(&array);

    ASSERT_EQ(parse_commands(&runner, &array, "save -eUTF-8"), CMDERR_NONE);
    ASSERT_EQ(array.count, 3);
    cmd = cmds->lookup(array.ptrs[0]);
    ASSERT_NONNULL(cmd);
    a = cmdargs_new((char**)array.ptrs + 1);
    EXPECT_EQ(do_parse_args(cmd, &a), ARGERR_OPTION_ARGUMENT_NOT_SEPARATE);
    EXPECT_EQ(a.flags[0], 'e');
    ptr_array_free(&array);
}

static void test_cached_command_new(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    const CommandRunner runner = cmdrunner_for_mode(e, INPUT_NORMAL, false);
    const CommandSet *cmds = runner.cmds;
    const char cmd_str[] = "open -t -e UTF-8 file.c inc.h";
    CachedCommand *cc = cached_command_new(&runner, cmd_str);
    ASSERT_NONNULL(cmds);
    ASSERT_NONNULL(cc);
    ASSERT_NONNULL(cc->cmd);
    EXPECT_PTREQ(cc->cmd, cmds->lookup("open"));
    EXPECT_STREQ(cc->cmd_str, cmd_str);
    ASSERT_EQ(cc->a.nr_args, 2);
    ASSERT_EQ(cc->a.nr_flag_args, 1);
    ASSERT_EQ(cc->a.nr_flags, 2);
    EXPECT_STREQ(cc->a.args[0], "UTF-8");
    EXPECT_STREQ(cc->a.args[1], "file.c");
    EXPECT_STREQ(cc->a.args[2], "inc.h");
    EXPECT_EQ(cc->a.flags[0], 't');
    EXPECT_EQ(cc->a.flags[1], 'e');
    EXPECT_TRUE(cmdargs_has_flag(&cc->a, 't'));
    EXPECT_TRUE(cmdargs_has_flag(&cc->a, 'e'));
    cached_command_free(cc);
    cached_command_free(NULL);

    static const char *const uncacheable[] = {
        "", // No command
        "zxcvbnm", // Invalid command
        "left; right", // Multiple commands
        "insert $DTE_HOME", // Variable expansion
        "insert -xyz321", // Invalid flags
        "alias 1 2 3", // Too many arguments
        "alias", // Too few arguments
    };

    for (size_t i = 0; i < ARRAYLEN(uncacheable); i++) {
        cc = cached_command_new(&runner, uncacheable[i]);
        ASSERT_NONNULL(cc);
        EXPECT_NULL(cc->cmd);
        cached_command_free(cc);
    }
}

static const char *escape_command_arg(String *buf, const char *arg, bool escape_tilde)
{
    string_clear(buf);
    string_append_escaped_arg(buf, arg, escape_tilde);
    return string_borrow_cstring(buf);
}

static void test_string_append_escaped_arg(TestContext *ctx)
{
    String buf = string_new(64);
    const char *str = escape_command_arg(&buf, "arg", false);
    EXPECT_STREQ(str, "arg");

    str = escape_command_arg(&buf, "arg-x.y:z", false);
    EXPECT_STREQ(str, "arg-x.y:z");

    str = escape_command_arg(&buf, "", false);
    EXPECT_STREQ(str, "''");

    str = escape_command_arg(&buf, " ", false);
    EXPECT_STREQ(str, "' '");

    str = escape_command_arg(&buf, "hello world", false);
    EXPECT_STREQ(str, "'hello world'");

    str = escape_command_arg(&buf, "line1\nline2\n", false);
    EXPECT_STREQ(str, "\"line1\\nline2\\n\"");

    str = escape_command_arg(&buf, " \t\r\n\x1F\x7F", false);
    EXPECT_STREQ(str, "\" \\t\\r\\n\\x1F\\x7F\"");

    str = escape_command_arg(&buf, "x ' y", false);
    EXPECT_STREQ(str, "\"x ' y\"");

    str = escape_command_arg(&buf, "\033[P", false);
    EXPECT_STREQ(str, "\"\\e[P\"");

    str = escape_command_arg(&buf, "\"''\"", false);
    EXPECT_STREQ(str, "\"\\\"''\\\"\"");

    str = escape_command_arg(&buf, "~/file with spaces", false);
    EXPECT_STREQ(str, "~/'file with spaces'");
    str = escape_command_arg(&buf, "~/file with spaces", true);
    EXPECT_STREQ(str, "'~/file with spaces'");

    str = escape_command_arg(&buf, "~/need \t\ndquotes", false);
    EXPECT_STREQ(str, "~/\"need \\t\\ndquotes\"");
    str = escape_command_arg(&buf, "~/need \t\ndquotes", true);
    EXPECT_STREQ(str, "\"~/need \\t\\ndquotes\"");

    str = escape_command_arg(&buf, "~/file-with-no-spaces", false);
    EXPECT_STREQ(str, "~/file-with-no-spaces");
    str = escape_command_arg(&buf, "~/file-with-no-spaces", true);
    EXPECT_STREQ(str, "\\~/file-with-no-spaces");

    str = escape_command_arg(&buf, "~/", false);
    EXPECT_STREQ(str, "~/");
    str = escape_command_arg(&buf, "~/", true);
    EXPECT_STREQ(str, "\\~/");

    string_free(&buf);
}

static void test_command_struct_layout(TestContext *ctx)
{
    const Command *cmd = find_normal_command("open");
    EXPECT_STREQ(cmd->name, "open");
    EXPECT_STREQ(cmd->flags, "e=gt");
    EXPECT_FALSE(cmd->allow_in_rc);
    EXPECT_UINT_EQ(cmd->min_args, 0);
    EXPECT_UINT_EQ(cmd->max_args, 0xFF);

    IGNORE_WARNING("-Wpedantic")
    EXPECT_NONNULL(cmd->cmd);
    UNIGNORE_WARNINGS
}

static void test_cmdargs_flagset_idx(TestContext *ctx)
{
    EXPECT_EQ(cmdargs_flagset_idx('A'), 1);
    EXPECT_EQ(cmdargs_flagset_idx('Z'), 26);
    EXPECT_EQ(cmdargs_flagset_idx('a'), 27);
    EXPECT_EQ(cmdargs_flagset_idx('z'), 52);
    EXPECT_EQ(cmdargs_flagset_idx('0'), 53);
    EXPECT_EQ(cmdargs_flagset_idx('1'), 54);
    EXPECT_EQ(cmdargs_flagset_idx('9'), 62);

    for (unsigned char c = '0', z = 'z'; c <= z; c++) {
        if (ascii_isalnum(c)) {
            const unsigned int idx = cmdargs_flagset_idx(c);
            EXPECT_TRUE(idx < 63);
            EXPECT_EQ(idx, base64_decode(c) + 1);
        }
    }
}

static void test_cmdargs_convert_flags(TestContext *ctx)
{
    static const FlagMapping map[] = {
        {'b', REPLACE_BASIC},
        {'c', REPLACE_CONFIRM},
        {'g', REPLACE_GLOBAL},
        {'i', REPLACE_IGNORE_CASE},
    };

    const CommandArgs a = {
        .flag_set = cmdargs_flagset_value('c') | cmdargs_flagset_value('g')
    };

    ReplaceFlags flags = cmdargs_convert_flags(&a, map, ARRAYLEN(map));
    EXPECT_EQ(flags, REPLACE_CONFIRM | REPLACE_GLOBAL);
}

static void test_add_alias(TestContext *ctx)
{
    const char name[] = "insert-foo";
    const char cmd[] = "insert -m foo";
    HashMap m = HASHMAP_INIT;
    add_alias(&m, name, cmd);
    EXPECT_STREQ(find_alias(&m, name), cmd);
    EXPECT_EQ(m.count, 1);

    remove_alias(&m, "insert-foo");
    EXPECT_NULL(find_alias(&m, name));
    EXPECT_EQ(m.count, 0);
    hashmap_free(&m, free);
}

static const TestEntry tests[] = {
    TEST(test_parse_command_arg),
    TEST(test_parse_commands),
    TEST(test_command_parse_error_to_string),
    TEST(test_find_normal_command),
    TEST(test_parse_args),
    TEST(test_cached_command_new),
    TEST(test_string_append_escaped_arg),
    TEST(test_command_struct_layout),
    TEST(test_cmdargs_flagset_idx),
    TEST(test_cmdargs_convert_flags),
    TEST(test_add_alias),
};

const TestGroup command_tests = TEST_GROUP(tests);
