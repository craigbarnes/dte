#include "test.h"
#include "commands.h"
#include "editor.h"
#include "error.h"
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
        {"def-mode _m; def-mode _m", "mode '_m' already exists"},
        {"errorfmt x (re) zz", "unknown substring name"},
        {"errorfmt x (re) file line", "invalid substring count"},
        {"exec -s sh -c 'kill -s USR1 $$'", "child received signal"},
        {"exec -s sh -c 'exit 44'", "child returned 44"},
        {"exec -s _z9kjdf_2dmm_92:j-a3d_xzkw::", "unable to exec"},
        {"exec -s -e errmsg sh -c 'echo X >&2; exit 9'", "child returned 9: \"X\""},
        {"ft -- -name ext", "invalid filetype name"},
        {"hi xyz red green blue", "too many colors"},
        {"hi xyz _invalid_", "invalid color or attribute"},
        {"line 0", "invalid line number"},
        {"line _", "invalid line number"},
        {"line 1:x", "invalid line number"},
        {"macro xyz", "unknown action"},
        {"match-bracket", "no character under cursor"},
        {"move-tab 0", "invalid tab position"},
        {"msg -np", "mutually exclusive"},
        {"msg -p 12", "mutually exclusive"},
        {"open -t filename", "can't be used with filename arguments"},
        {"open /non/exi/ste/nt/file_Name", "error opening"},
        {"open /dev/null", "not a regular file"},
        {"option c auto-indent true syntax", "missing option value"},
        {"option c filetype c", "filetype cannot be set via option command"},
        {"option , auto-indent true", "invalid filetype"},
        {"option -r '' auto-indent true", "empty pattern"},
        {"paste -ac", "mutually exclusive"},
        {"quit xyz", "not a valid integer"},
        {"quit 9000", "exit code should be between"},
        {"redo 8", "nothing to redo"},
        {"repeat x up", "not a valid repeat count"},
        {"repeat 2 invalid-cmd", "no such command: invalid-cmd"},
        {"replace '' x", "must contain at least 1 character"},
        {"replace e5fwHgHuCFVZd x", "not found"},
        {"save -bB", "flags -b and -B can't be used together"},
        {"save -du", "flags -d and -u can't be used together"},
        {"save ''", "empty filename"},
        {"save", "no filename"},
        {"search -nw", "mutually exclusive"},
        {"search -p str", "mutually exclusive"},
        {"search e5fwHgHuCFVZd", "not found"},
        {"set filetype", "not boolean"},
        {"set filetype ' '", "invalid filetype name"},
        {"set tab-width s", "integer value for tab-width expected"},
        {"set indent-width 9", "must be in 1-8 range"},
        {"set emulate-tab x", "invalid value"},
        {"set case-sensitive-search z", "invalid value"},
        {"set ws-error A", "invalid flag"},
        {"set non-existent 1", "no such option"},
        {"set -g filetype c", "not global"},
        {"set -l statusline-right _", "not local"},
        {"set 1 2 3", "one or even number of arguments expected"},
        {"set statusline-left %", "format character expected"},
        {"set statusline-left %!", "invalid format character"},
        {"setenv DTE_VERSION 0.1", "cannot be changed"},
        {"shift x", "invalid number"},
        {"shift 0", "must be non-zero"},
        {"show -c alias", "requires 2 arguments"},
        {"show zzz", "invalid argument"},
        {"show bind M-M-M---", "invalid key string"},
        {"show buffer x", "doesn't take extra arguments"},
        {"show builtin a78sd8623d7k", "no built-in config"},
        {"show cursor xyz", "no cursor entry"},
        {"show option 31ldx92kjsd8", "invalid option name"},
        {"show wsplit m2wz9u263t8a", "invalid window"},
        {"toggle ws-error", "requires arguments"},
        {"view z", "invalid view index"},
        {"view 0", "invalid view index"},
        {"wrap-paragraph _", "invalid paragraph width"},
        {"wrap-paragraph 90000", "width must be between"},
        {"wsplit -t file", "flags -n and -t can't be used with filename"},

        // Error strings produced by arg_parse_error_msg():
        {"bind -nnnnnnnnnn C-k eol", "too many options given"},
        {"bind -Z C-k eol", "invalid option -Z"},
        {"bind -Tsearch C-k eol", "option -T must be given separately"},
        {"bind -T ", "option -T requires an argument"},
        {"bind 1 2 3", "too many arguments"},
        {"bind", "too few arguments"},
    };

    EditorState *e = ctx->userdata;
    ASSERT_NONNULL(window_open_empty_buffer(e->window));

    FOR_EACH_I(i, tests) {
        const char *cmd = tests[i].cmd_str;
        const char *substr = tests[i].expected_error_substr;
        ASSERT_NONNULL(cmd);
        ASSERT_NONNULL(substr);
        ASSERT_TRUE(substr[0] != '\0');
        ASSERT_TRUE(substr[1] != '\0');
        ASSERT_TRUE(!ascii_isupper(substr[0]));

        clear_error();
        EXPECT_FALSE(handle_normal_command(e, cmd, false));
        bool is_error = false;
        const char *msg = get_msg(&is_error);
        ASSERT_NONNULL(msg);
        EXPECT_TRUE(is_error);

        // Check for substring in error message (ignoring capitalization)
        const char *found = strstr(msg, substr + 1);
        if (likely(found && found != msg)) {
            test_pass(ctx);
            EXPECT_EQ(ascii_tolower(found[-1]), substr[0]);
        } else {
            TEST_FAIL (
                "Test #%zu: substring \"%s\" not found in message \"%s\"",
                i + 1, substr, msg
            );
        }
    }

    window_close_current_view(e->window);
    set_view(e->window->view);

    // Special case errors produced by run_command():
    // ----------------------------------------------

    CommandRunner runner = normal_mode_cmdrunner(e);
    runner.lookup_alias = NULL;
    clear_error();
    EXPECT_FALSE(handle_command(&runner, "_xyz"));
    bool is_error = false;
    const char *msg = get_msg(&is_error);
    EXPECT_STREQ(msg, "No such command: _xyz");
    EXPECT_TRUE(is_error);

    runner.lookup_alias = dummy_lookup_alias;
    clear_error();
    EXPECT_FALSE(handle_command(&runner, "_abc"));
    msg = get_msg(&is_error);
    EXPECT_TRUE(msg && str_has_prefix(msg, "Parsing alias _abc:"));
    EXPECT_TRUE(is_error);

    EXPECT_NULL(current_config.file);
    current_config.file = "example";
    current_config.line = 1;
    runner.lookup_alias = NULL;
    clear_error();
    EXPECT_FALSE(handle_command(&runner, "quit"));
    msg = get_msg(&is_error);
    EXPECT_STREQ(msg, "example:1: Command quit not allowed in config file");
    EXPECT_TRUE(is_error);
    current_config.file = NULL;

    // TODO: Cover alias recursion overflow check in run_commands()
}

static const TestEntry tests[] = {
    TEST(test_normal_command_errors),
};

const TestGroup error_tests = TEST_GROUP(tests);
