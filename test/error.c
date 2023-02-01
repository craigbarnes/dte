#include "test.h"
#include "commands.h"
#include "editor.h"
#include "error.h"
#include "util/ascii.h"
#include "window.h"

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
        {"cd", "too few arguments"},
        {"cd ''", "directory argument cannot be empty"},
        {"cd /non-existent/dir/_x_29_", "changing directory failed"},
        {"compile dfa34dazzdf2 echo", "no such error parser"},
        {"cursor xyz bar #f40", "invalid mode argument"},
        {"cursor insert _ #f40", "invalid cursor type"},
        {"cursor insert bar blue", "invalid cursor color"},
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
        {"option c auto-indent true syntax", "missing option value"},
        {"option c filetype c", "filetype cannot be set via option command"},
        {"paste -ac", "mutually exclusive"},
        {"quit xyz", "not a valid integer"},
        {"quit 9000", "exit code should be between"},
        {"repeat x up", "not a valid repeat count"},
        {"save -bB", "flags -b and -B can't be used together"},
        {"save -du", "flags -d and -u can't be used together"},
        {"save ''", "empty filename"},
        {"save", "no filename"},
        {"search -nw", "mutually exclusive"},
        {"search -p str", "mutually exclusive"},
        {"set filetype", "not boolean"},
        {"set 1 2 3", "one or even number of arguments expected"},
        {"setenv DTE_VERSION 0.1", "cannot be changed"},
        {"shift x", "invalid number"},
        {"shift 0", "must be non-zero"},
        {"show -c alias", "requires 2 arguments"},
        {"show zzz", "invalid argument"},
        {"show bind M-M-M---", "invalid key string"},
        {"view z", "invalid view index"},
        {"view 0", "invalid view index"},
        {"wrap-paragraph _", "invalid paragraph width"},
        {"wrap-paragraph 90000", "width must be between"},
        {"wsplit -t file", "flags -n and -t can't be used with filename"},
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

        clear_error();
        EXPECT_FALSE(handle_normal_command(e, cmd, false));
        bool is_error = false;
        const char *msg = get_msg(&is_error);
        ASSERT_NONNULL(msg);
        EXPECT_TRUE(is_error);

        // Check for substring in error message (ignoring capitalization)
        const char *found = strstr(msg, substr + 1);
        if (unlikely(!found || found == msg)) {
            TEST_FAIL("substring '%s' not found in message '%s'", substr, msg);
            continue;
        }
        EXPECT_EQ(ascii_tolower(found[-1]), substr[0]);
    }

    window_close_current_view(e->window);
    set_view(e->window->view);
}

static const TestEntry tests[] = {
    TEST(test_normal_command_errors),
};

const TestGroup error_tests = TEST_GROUP(tests);
