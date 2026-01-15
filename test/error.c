#include "test.h"
#include "command/error.h"
#include "commands.h"
#include "editor.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "window.h"

static const char *dummy_lookup_alias(const EditorState *e, const char *name)
{
    BUG_ON(!e);
    BUG_ON(!name);
    return "insert \"...";
}

static void test_normal_command_errors(TestContext *ctx)
{
    static const struct {
        const char *cmd_str;
        const char *expected_error_substr;
    } tests[] = {
        // Note: do not include error substrings produced by strerror(3)
        {"non__existent__command", "no such command or alias"},
        {"alias '' up", "empty alias name"},
        {"alias -- -name up", "alias name cannot begin with '-'"},
        {"alias 'x y' up", "invalid byte in alias name"},
        {"alias open close", "can't replace existing command"},
        {"alias a b; alias b a; a; alias a; alias b", "alias recursion limit reached"},
        {"bind C-C-C", "invalid key string"},
        {"bind -T invalid-mode C-k eol", "can't bind key in unknown mode"},
        {"cd", "too few arguments"},
        {"cd ''", "directory argument cannot be empty"},
        {"cd /non-existent/dir/_x_29_", "changing directory failed"},
        {"compile dfa34dazzdf2 echo", "no such error parser"},
        {"cursor xyz bar #f40", "invalid mode argument"},
        {"cursor insert _ #f40", "invalid cursor type"},
        {"cursor insert bar blue", "invalid cursor color"},
        {"def-mode normal", "mode 'normal' already exists"},
        {"def-mode command", "mode 'command' already exists"},
        {"def-mode search", "mode 'search' already exists"},
        {"def-mode -- -", "mode name can't be empty or start with '-'"},
        {"def-mode ''", "mode name can't be empty or start with '-'"},
        {"def-mode xz qrst9", "unknown fallthrough mode 'qrst9'"},
        {"def-mode xz search", "unable to use 'search' as fall-through mode"},
        {"def-mode _m; def-mode _m", "mode '_m' already exists"},
        {"errorfmt x (re) zz", "unknown substring name"},
        {"errorfmt x (re) file line", "expected 2 subexpressions in regex, but found 1: '(re)'"},
        {"exec -s sh -c 'kill -s USR1 $$'", "child received signal"},
        {"exec -s sh -c 'exit 44'", "child returned 44"},
        {"exec -s _z9kjdf_2dmm_92:j-a3d_xzkw::", "unable to exec"},
        {"exec -s -e errmsg sh -c 'echo X >&2; exit 9'", "child returned 9: \"X\""},
        {"exec -s -i invalid true", "invalid action for -i: 'invalid'"},
        {"exec -s -o invalid true", "invalid action for -o: 'invalid'"},
        {"exec -s -e invalid true", "invalid action for -e: 'invalid'"},
        {"ft -- -name ext", "invalid filetype name"},
        {"hi xyz red green blue", "too many colors"},
        {"hi xyz _invalid_", "invalid color or attribute"},
        {"include -b nonexistent", "no built-in config with name 'nonexistent'"},
        {"include /.n-o-n-e-x-i-s-t-e-n-t/_/az_..3", "error reading"},
        {"line 0", "invalid line number"},
        {"line _", "invalid line number"},
        {"line 1:x", "invalid line number"},
        {"macro xyz", "unknown action"},
        {"match-bracket", "no character under cursor"},
        {"mode inValiD", "unknown mode 'inValiD'"},
        {"move-tab 0", "invalid tab position"},
        {"msg -p 12", "flags [-n|-p] can't be used with [number] argument"},
        {"open -t filename", "can't be used with filename arguments"},
        {"open /non/exi/ste/nt/file_Name", "directory does not exist"},
        {"open /dev/null", "not a regular file"},
        {"open ''", "empty filename not allowed"},
        {"option c auto-indent true syntax", "missing option value"},
        {"option c filetype c", "filetype cannot be set via option command"},
        {"option c invalid 1", "no such option"},
        {"option c tab-bar true", "is not local"},
        {"option c detect-indent 1,0", "invalid flag"},
        {"option c detect-indent 9", "invalid flag"},
        {"option c indent-width 0", "must be in 1-8 range"},
        {"option c indent-width 9", "must be in 1-8 range"},
        {"option c auto-indent xyz", "invalid value"},
        {"option , auto-indent true", "invalid filetype"},
        {"option '' auto-indent true", "first argument cannot be empty"},
        {"option -r '' auto-indent true", "first argument cannot be empty"},
        {"quit xyz", "exit code should be an integer between 0 and 125; got 'xyz'"},
        {"quit 126", "exit code should be an integer between 0 and 125; got '126'"},
        {"redo 8", "nothing to redo"},
        {"redo A", "invalid change id"},
        {"repeat x up", "not a valid repeat count"},
        {"repeat 2 invalid-cmd", "no such command: invalid-cmd"},
        {"replace '' x", "must contain at least 1 character"},
        {"replace e5fwHgHuCFVZd x", "not found"},
        {"replace -c a b", "-c flag unavailable in headless mode"},
        {"save ''", "empty filename"},
        {"save", "no filename"},
        {"save /dev/", "will not overwrite directory"},
        {"search -nw", "mutually exclusive"},
        {"search -p str", "mutually exclusive"},
        {"search e5fwHgHuCFVZd", "not found"},
        {"set filetype", "not boolean"},
        {"set filetype ' '", "invalid filetype name"},
        {"set tab-width s", "integer value for tab-width expected"},
        {"set indent-width 9", "must be in 1-8 range"},
        {"set emulate-tab x", "invalid value"},
        {"set msg-compile xyz", "invalid value for msg-compile; expected: A, B, C"},
        {"set msg-tag xyz", "invalid value for msg-tag; expected: A, B, C"},
        {"set newline xyz", "invalid value for newline; expected: unix, dos"},
        {
            "set case-sensitive-search z",
            "invalid value for case-sensitive-search; expected: false, true, auto"
        },
        {
            "set save-unmodified xyz",
            "invalid value for save-unmodified; expected: none, touch, full"
        },
        {
            "set window-separator xyz",
            "invalid value for window-separator; expected: blank, bar"
        },
        {
            "set ws-error A",
            "invalid flag 'A' for ws-error; expected: space-indent,"
            " space-align, tab-indent, tab-after-indent, special,"
            " auto-indent, trailing, all-trailing"
        },
        {"set non-existent 1", "no such option"},
        {"set -g filetype c", "not global"},
        {"set -l statusline-right _", "not local"},
        {"set 1 2 3", "one or even number of arguments expected"},
        {"set statusline-left %", "format character expected"},
        {"set statusline-left %!", "invalid format character"},
        {"set filesize-limit 1M", "invalid filesize"},
        {"set filesize-limit 1Mi", "invalid filesize"},
        {"set filesize-limit 1MB", "invalid filesize"},
        {"set filesize-limit 1MIB", "invalid filesize"},
        {"set filesize-limit 1Mib", "invalid filesize"},
        {"setenv DTE_VERSION 0.1", "cannot be changed"},
        {"shift x", "invalid number"},
        {"show -c alias", "requires 2 arguments"},
        {"show zzz", "invalid argument"},
        {"show bind M-M-M---", "invalid key string"},
        {"show buffer x", "doesn't take extra arguments"},
        {"show builtin a78sd8623d7k", "no built-in config"},
        {"show cursor xyz", "no cursor entry"},
        {"show option 31ldx92kjsd8", "invalid option name"},
        {"show wsplit m2wz9u263t8a", "invalid window"},
        {"suspend", "unavailable in headless mode"},
        {"toggle ws-error", "requires arguments"},
        {"view z", "invalid view index"},
        {"view 0", "invalid view index"},
        {"wrap-paragraph _", "invalid paragraph width"},
        {"wrap-paragraph 90000", "width must be between"},
        {"wsplit -t file", "flags [-n|-t] can't be used with [filename] arguments"},
        {"wsplit -t; wresize nonnum; wclose", "invalid resize value"},

        // Error strings produced by command_parse_error_to_string():
        {"x '", "command syntax error: unclosed '"},
        {"x \"", "command syntax error: unclosed \""},
        {"x \"\\", "command syntax error: unexpected EOF"},

        // Error strings produced by arg_parse_error_msg():
        {"bind -nnnnnnnnnnnnnn C-k eol", "too many options given"},
        {"bind -Z C-k eol", "invalid option -Z"},
        {"bind -Tsearch C-k eol", "option -T must be given separately"},
        {"bind -T ", "option -T requires an argument"},
        {"bind 1 2 3", "too many arguments"},
        {"bind", "too few arguments"},

        // These are tested last, since some of the commands succeed and
        // affect the state of the Buffer (i.e. the undo history) and some
        // are dependent on it (and cannot be reordered)
        {"insert 1; undo; redo 900", "there is only 1 possible change to redo"},
        {"insert 2; undo; redo 900", "there are only 2 possible changes to redo"},
        {"insert x; close; undo", "close without saving"},
        {"insert x; wclose; undo", "close window without saving"},
        {"insert x; quit; undo", "quit without saving"},
        {"insert x; close -p; undo", "-p flag unavailable in headless mode"},
        {"insert x; wclose -p; undo", "-p flag unavailable in headless mode"},
        {"insert x; quit -p; undo", "-p flag unavailable in headless mode"},
        {"insert x; match-bracket; undo", "character under cursor not matchable"},
        {"insert {{}; match-bracket; undo", "no matching bracket found"},
        {"repeat 4100 insert x; search -w; undo", "word under cursor too long"},
    };

    EditorState *e = ctx->userdata;
    ErrorBuffer *ebuf = &e->err;
    ASSERT_NONNULL(window_open_empty_buffer(e->window));

    FOR_EACH_I(i, tests) {
        const char *cmd = tests[i].cmd_str;
        const char *substr = tests[i].expected_error_substr;
        ASSERT_NONNULL(cmd);
        ASSERT_NONNULL(substr);
        ASSERT_NE(substr[0], '\0');
        ASSERT_NE(substr[1], '\0');
        ASSERT_TRUE(!ascii_isupper(substr[0]));

        clear_error(ebuf);
        EXPECT_FALSE(handle_normal_command(e, cmd, false));
        EXPECT_NE(ebuf->buf[0], '\0');
        EXPECT_TRUE(ebuf->is_error);

        // Check for substring in error message (ignoring capitalization)
        const char *found = strstr(ebuf->buf, substr + 1);
        if (likely(found && found != ebuf->buf)) {
            test_pass(ctx);
            EXPECT_EQ(ascii_tolower(found[-1]), substr[0]);
        } else {
            TEST_FAIL (
                "Test #%zu: substring \"%s\" not found in message \"%s\"",
                i + 1, substr, ebuf->buf
            );
        }
    }

    window_close_current_view(e->window);
    set_view(e->window->view);

    // Special case errors produced by run_command():
    // ----------------------------------------------

    CommandRunner runner = normal_mode_cmdrunner(e);
    runner.lookup_alias = NULL;
    clear_error(ebuf);
    EXPECT_FALSE(handle_command(&runner, "_xyz"));
    EXPECT_STREQ(ebuf->buf, "No such command: _xyz");
    EXPECT_TRUE(ebuf->is_error);

    runner.lookup_alias = dummy_lookup_alias;
    clear_error(ebuf);
    EXPECT_FALSE(handle_command(&runner, "_abc"));
    EXPECT_TRUE(str_has_prefix(ebuf->buf, "Parsing alias _abc:"));
    EXPECT_TRUE(ebuf->is_error);

    EXPECT_NULL(ebuf->config_filename);
    ebuf->config_filename = "example";
    ebuf->config_line = 1;
    runner.lookup_alias = NULL;
    clear_error(ebuf);
    EXPECT_FALSE(handle_command(&runner, "quit"));
    EXPECT_STREQ(ebuf->buf, "example:1: Command quit not allowed in config file");
    EXPECT_TRUE(ebuf->is_error);
    ebuf->config_filename = NULL;

    // Special case errors produced by exec_config():
    // ----------------------------------------------

    // This is a special case because handle_command() currently returns true,
    // since cmd_include() only indicates failure for I/O errors and not for
    // failing commands in the loaded file
    clear_error(ebuf);
    EXPECT_TRUE(handle_command(&runner, "include test/data/recursive.dterc"));
    EXPECT_STREQ(ebuf->buf, "test/data/recursive.dterc:1: config recursion limit reached");
    EXPECT_TRUE(ebuf->is_error);
}

static const TestEntry tests[] = {
    TEST(test_normal_command_errors),
};

const TestGroup error_tests = TEST_GROUP(tests);
