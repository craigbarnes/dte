#include <limits.h>
#include "test.h"
#include "command/args.h"
#include "command/env.h"
#include "command/parse.h"
#include "command/run.h"
#include "command/serialize.h"
#include "commands.h"
#include "editor.h"
#include "util/ascii.h"
#include "util/debug.h"

static void test_parse_command_arg(void)
{
    // Single, unquoted argument
    char *arg = parse_command_arg(STRN("arg"), false);
    EXPECT_STREQ(arg, "arg");
    free(arg);

    // Two unquoted, space-separated arguments
    arg = parse_command_arg(STRN("hello world"), false);
    EXPECT_STREQ(arg, "hello");
    free(arg);

    // Two unquoted, tab-separated arguments
    arg = parse_command_arg(STRN("hello\tworld"), false);
    EXPECT_STREQ(arg, "hello");
    free(arg);

    // Unquoted argument preceded by whitespace
    arg = parse_command_arg(STRN(" x"), false);
    EXPECT_STREQ(arg, "");
    free(arg);

    // Single-quoted argument, including whitespace
    arg = parse_command_arg(STRN("'  foo ' .."), false);
    EXPECT_STREQ(arg, "  foo ");
    free(arg);

    // Several adjacent, quoted strings forming a single argument
    arg = parse_command_arg(STRN("\"foo\"'bar'' baz '\"etc\"."), false);
    EXPECT_STREQ(arg, "foobar baz etc.");
    free(arg);

    // Control character escapes in a double-quoted string
    arg = parse_command_arg(STRN("\"\\a\\b\\t\\n\\v\\f\\r\""), false);
    EXPECT_STREQ(arg, "\a\b\t\n\v\f\r");
    free(arg);

    // Backslash escape sequence in a double-quoted string
    arg = parse_command_arg(STRN("\"\\\\\""), false);
    EXPECT_STREQ(arg, "\\");
    free(arg);

    // Double-quote escape sequence in a double-quoted string
    arg = parse_command_arg(STRN("\"\\\"\""), false);
    EXPECT_STREQ(arg, "\"");
    free(arg);

    // Escape character escape sequence in a double-quoted string
    arg = parse_command_arg(STRN("\"\\e[0m\""), false);
    EXPECT_STREQ(arg, "\033[0m");
    free(arg);

    // Unrecognized escape sequence in a double-quoted string
    arg = parse_command_arg(STRN("\"\\z\""), false);
    EXPECT_STREQ(arg, "\\z");
    free(arg);

    // Hexadecimal escape sequences in a double-quoted string
    arg = parse_command_arg(STRN("\"\\x1B[31m\\x7E\\x2f\\x1b[0m\""), false);
    EXPECT_STREQ(arg, "\x1B[31m~/\x1B[0m");
    free(arg);

    // 4-digit Unicode escape sequence
    arg = parse_command_arg(STRN("\"\\u148A\""), false);
    EXPECT_STREQ(arg, "\xE1\x92\x8A");
    free(arg);

    // 8-digit Unicode escape sequence
    arg = parse_command_arg(STRN("\"\\U0001F4A4\""), false);
    EXPECT_STREQ(arg, "\xF0\x9F\x92\xA4");
    free(arg);

    // "\U" escape sequence terminated by non-hexadecimal character
    arg = parse_command_arg(STRN("\"\\U1F4A4...\""), false);
    EXPECT_STREQ(arg, "\xF0\x9F\x92\xA4...");
    free(arg);

    // Unsupported, escape-like sequences in a single-quoted string
    arg = parse_command_arg(STRN("'\\t\\n'"), false);
    EXPECT_STREQ(arg, "\\t\\n");
    free(arg);

    // Single-quoted, empty string
    arg = parse_command_arg(STRN("''"), false);
    EXPECT_STREQ(arg, "");
    free(arg);

    // Double-quoted, empty string
    arg = parse_command_arg(STRN("\"\""), false);
    EXPECT_STREQ(arg, "");
    free(arg);

    // NULL input with zero length
    arg = parse_command_arg(NULL, 0, false);
    EXPECT_STREQ(arg, "");
    free(arg);

    // Empty input
    arg = parse_command_arg("", 1, false);
    EXPECT_STREQ(arg, "");
    free(arg);

    arg = parse_command_arg(STRN("\"\\u148A\"xyz'foo'\"\\x5A\"\\;\t."), false);
    EXPECT_STREQ(arg, "\xE1\x92\x8AxyzfooZ;");
    free(arg);
}

static void test_parse_commands(void)
{
    PointerArray array = PTR_ARRAY_INIT;
    EXPECT_EQ(parse_commands(&array, " left  -c;;"), CMDERR_NONE);
    EXPECT_EQ(array.count, 5);
    EXPECT_STREQ(array.ptrs[0], "left");
    EXPECT_STREQ(array.ptrs[1], "-c");
    EXPECT_NULL(array.ptrs[2]);
    EXPECT_NULL(array.ptrs[3]);
    EXPECT_NULL(array.ptrs[4]);
    ptr_array_free(&array);

    EXPECT_EQ(parse_commands(&array, "save -e UTF-8 file.c; close -q"), CMDERR_NONE);
    EXPECT_EQ(array.count, 8);
    EXPECT_STREQ(array.ptrs[0], "save");
    EXPECT_STREQ(array.ptrs[1], "-e");
    EXPECT_STREQ(array.ptrs[2], "UTF-8");
    EXPECT_STREQ(array.ptrs[3], "file.c");
    EXPECT_NULL(array.ptrs[4]);
    EXPECT_STREQ(array.ptrs[5], "close");
    EXPECT_STREQ(array.ptrs[6], "-q");
    EXPECT_NULL(array.ptrs[7]);
    ptr_array_free(&array);

    EXPECT_EQ(parse_commands(&array, "\n ; ; \t\n "), CMDERR_NONE);
    EXPECT_EQ(array.count, 3);
    EXPECT_NULL(array.ptrs[0]);
    EXPECT_NULL(array.ptrs[1]);
    EXPECT_NULL(array.ptrs[2]);
    ptr_array_free(&array);

    EXPECT_EQ(parse_commands(&array, ""), CMDERR_NONE);
    EXPECT_EQ(array.count, 1);
    EXPECT_NULL(array.ptrs[0]);
    ptr_array_free(&array);

    EXPECT_EQ(parse_commands(&array, "insert '... "), CMDERR_UNCLOSED_SQUOTE);
    ptr_array_free(&array);

    EXPECT_EQ(parse_commands(&array, "insert \" "), CMDERR_UNCLOSED_DQUOTE);
    ptr_array_free(&array);

    EXPECT_EQ(parse_commands(&array, "insert \"\\\" "), CMDERR_UNCLOSED_DQUOTE);
    ptr_array_free(&array);

    EXPECT_EQ(parse_commands(&array, "insert \\"), CMDERR_UNEXPECTED_EOF);
    ptr_array_free(&array);
}

static void test_command_parse_error_to_string(void)
{
    const char *str = command_parse_error_to_string(CMDERR_UNCLOSED_SQUOTE);
    EXPECT_STREQ(str, "unclosed '");
    str = command_parse_error_to_string(CMDERR_UNCLOSED_DQUOTE);
    EXPECT_STREQ(str, "unclosed \"");
    str = command_parse_error_to_string(CMDERR_UNEXPECTED_EOF);
    EXPECT_STREQ(str, "unexpected EOF");
}

static void test_expand_builtin_env(void)
{
    char *value = NULL;
    EXPECT_TRUE(expand_builtin_env("DTE_HOME", &value));
    EXPECT_STREQ(value, editor.user_config_dir);
    free(value);

    EXPECT_TRUE(expand_builtin_env("FILE", &value));
    EXPECT_NULL(value);
    EXPECT_TRUE(expand_builtin_env("FILETYPE", &value));
    EXPECT_NULL(value);
    EXPECT_TRUE(expand_builtin_env("LINENO", &value));
    EXPECT_NULL(value);
    EXPECT_TRUE(expand_builtin_env("WORD", &value));
    EXPECT_NULL(value);
}

static void test_find_normal_command(void)
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

static void test_parse_args(void)
{
    const char *cmd_str = "open -g file.c file.h *.mk -e UTF-8";
    PointerArray array = PTR_ARRAY_INIT;
    ASSERT_EQ(parse_commands(&array, cmd_str), CMDERR_NONE);
    ASSERT_EQ(array.count, 8);

    const Command *cmd = find_normal_command(array.ptrs[0]);
    ASSERT_NONNULL(cmd);
    EXPECT_STREQ(cmd->name, "open");

    CommandArgs a = {.args = (char**)array.ptrs + 1};
    ASSERT_EQ(do_parse_args(cmd, &a), 0);
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
    ASSERT_EQ(parse_commands(&array, cmd_str), CMDERR_NONE);
    ASSERT_EQ(array.count, 8);
    EXPECT_STREQ(array.ptrs[0], "bind");
    EXPECT_STREQ(array.ptrs[1], "1");
    EXPECT_STREQ(array.ptrs[6], "-6");
    EXPECT_NULL(array.ptrs[7]);

    cmd = find_normal_command(array.ptrs[0]);
    ASSERT_NONNULL(cmd);
    EXPECT_STREQ(cmd->name, "bind");
    EXPECT_EQ(cmd->max_args, 2);
    EXPECT_EQ(cmd->flags[0], '-');

    a = (CommandArgs){.args = (char**)array.ptrs + 1, .flags = "TEST"};
    ASSERT_EQ(do_parse_args(cmd, &a), ARGERR_TOO_MANY_ARGUMENTS);
    EXPECT_EQ(a.nr_args, 6);
    EXPECT_EQ(a.nr_flags, 0);
    EXPECT_EQ(a.nr_flag_args, 0);
    EXPECT_EQ(a.flags[0], '\0');
    EXPECT_EQ(a.flag_set, 0);
    ptr_array_free(&array);

    cmd_str = "open \"-\\xff\"";
    ASSERT_EQ(parse_commands(&array, cmd_str), CMDERR_NONE);
    ASSERT_EQ(array.count, 3);
    EXPECT_STREQ(array.ptrs[0], "open");
    EXPECT_STREQ(array.ptrs[1], "-\xff");
    EXPECT_NULL(array.ptrs[2]);

    cmd = find_normal_command(array.ptrs[0]);
    ASSERT_NONNULL(cmd);
    EXPECT_STREQ(cmd->name, "open");

    a = (CommandArgs){.args = (char**)array.ptrs + 1};
    ASSERT_EQ(do_parse_args(cmd, &a), ARGERR_INVALID_OPTION | 0xFF00);
    EXPECT_EQ(a.nr_args, 0);
    EXPECT_EQ(a.nr_flags, 0);
    EXPECT_EQ(a.nr_flag_args, 0);
    EXPECT_EQ(a.flags[0], '\0');
    EXPECT_EQ(a.flag_set, 0);
    ptr_array_free(&array);

    ASSERT_EQ(parse_commands(&array, "save -e"), CMDERR_NONE);
    ASSERT_EQ(array.count, 3);
    cmd = find_normal_command(array.ptrs[0]);
    ASSERT_NONNULL(cmd);
    a = (CommandArgs){.args = (char**)array.ptrs + 1};
    ASSERT_EQ(do_parse_args(cmd, &a), ARGERR_OPTION_ARGUMENT_MISSING | 0x6500);
    ptr_array_free(&array);

    ASSERT_EQ(parse_commands(&array, "save -eUTF-8"), CMDERR_NONE);
    ASSERT_EQ(array.count, 3);
    cmd = find_normal_command(array.ptrs[0]);
    ASSERT_NONNULL(cmd);
    a = (CommandArgs){.args = (char**)array.ptrs + 1};
    ASSERT_EQ(do_parse_args(cmd, &a), ARGERR_OPTION_ARGUMENT_NOT_SEPARATE | 0x6500);
    ptr_array_free(&array);
}

static void test_escape_command_arg(void)
{
    char *str = escape_command_arg("arg", false);
    EXPECT_STREQ(str, "arg");
    free(str);

    str = escape_command_arg("arg-x.y:z", false);
    EXPECT_STREQ(str, "arg-x.y:z");
    free(str);

    str = escape_command_arg("", false);
    EXPECT_STREQ(str, "''");
    free(str);

    str = escape_command_arg(" ", false);
    EXPECT_STREQ(str, "' '");
    free(str);

    str = escape_command_arg("hello world", false);
    EXPECT_STREQ(str, "'hello world'");
    free(str);

    str = escape_command_arg("line1\nline2\n", false);
    EXPECT_STREQ(str, "\"line1\\nline2\\n\"");
    free(str);

    str = escape_command_arg(" \t\r\n\x1F\x7F", false);
    EXPECT_STREQ(str, "\" \\t\\r\\n\\x1F\\x7F\"");
    free(str);

    str = escape_command_arg("x ' y", false);
    EXPECT_STREQ(str, "\"x ' y\"");
    free(str);

    str = escape_command_arg("\033[P", false);
    EXPECT_STREQ(str, "\"\\e[P\"");
    free(str);

    str = escape_command_arg("\"''\"", false);
    EXPECT_STREQ(str, "\"\\\"''\\\"\"");
    free(str);

    str = escape_command_arg("~/file with spaces", false);
    EXPECT_STREQ(str, "~/'file with spaces'");
    free(str);
    str = escape_command_arg("~/file with spaces", true);
    EXPECT_STREQ(str, "'~/file with spaces'");
    free(str);

    str = escape_command_arg("~/need \t\ndquotes", false);
    EXPECT_STREQ(str, "~/\"need \\t\\ndquotes\"");
    free(str);
    str = escape_command_arg("~/need \t\ndquotes", true);
    EXPECT_STREQ(str, "\"~/need \\t\\ndquotes\"");
    free(str);

    str = escape_command_arg("~/file-with-no-spaces", false);
    EXPECT_STREQ(str, "~/file-with-no-spaces");
    free(str);
    str = escape_command_arg("~/file-with-no-spaces", true);
    EXPECT_STREQ(str, "\\~/file-with-no-spaces");
    free(str);

    str = escape_command_arg("~/", false);
    EXPECT_STREQ(str, "~/");
    free(str);
    str = escape_command_arg("~/", true);
    EXPECT_STREQ(str, "\\~/");
    free(str);

    String s = STRING_INIT;
    string_append_escaped_arg(&s, "~/", true);
    str = string_steal_cstring(&s);
    EXPECT_STREQ(str, "\\~/");
    free(str);
}

static void test_command_struct_layout(void)
{
    const Command *cmd = find_normal_command("filter");
    EXPECT_STREQ(cmd->name, "filter");
    EXPECT_STREQ(cmd->flags, "-l");
    EXPECT_UINT_EQ(cmd->min_args, 1);
    EXPECT_UINT_EQ(cmd->max_args, 0xFF);

    IGNORE_WARNING("-Wpedantic")
    EXPECT_NONNULL(cmd->cmd);
    UNIGNORE_WARNINGS
}

static void test_cmdargs_flagset_idx(void)
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

static const TestEntry tests[] = {
    TEST(test_parse_command_arg),
    TEST(test_parse_commands),
    TEST(test_command_parse_error_to_string),
    TEST(test_expand_builtin_env),
    TEST(test_find_normal_command),
    TEST(test_parse_args),
    TEST(test_escape_command_arg),
    TEST(test_command_struct_layout),
    TEST(test_cmdargs_flagset_idx),
};

const TestGroup command_tests = TEST_GROUP(tests);
