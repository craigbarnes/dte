#include <langinfo.h>
#include <limits.h>
#include <locale.h>
#include <string.h>
#include "test.h"
#include "../src/bind.h"
#include "../src/buffer.h"
#include "../src/command.h"
#include "../src/editor.h"
#include "../src/regexp.h"
#include "../src/util/path.h"
#include "../src/util/xmalloc.h"

void test_cmdline(void);
void test_command(void);
void test_editorconfig(void);
void test_encoding(void);
void test_filetype(void);
void test_syntax(void);
void test_terminal(void);
void test_util(void);
void init_headless_mode(void);
void test_exec_config(void);

static void test_relative_filename(void)
{
    static const struct rel_test {
        const char *cwd, *path, *result;
    } tests[] = { // NOTE: at most 2 ".." components allowed in relative name
        { "/", "/", "/" },
        { "/", "/file", "file" },
        { "/a/b/c/d", "/a/b/file", "../../file" },
        { "/a/b/c/d/e", "/a/b/file", "/a/b/file" },
        { "/a/foobar", "/a/foo/file", "../foo/file" },
    };
    FOR_EACH_I(i, tests) {
        const struct rel_test *t = &tests[i];
        char *result = relative_filename(t->path, t->cwd);
        IEXPECT_STREQ(t->result, result);
        free(result);
    }
}

static void test_detect_indent(void)
{
    EXPECT_FALSE(editor.options.detect_indent);
    EXPECT_FALSE(editor.options.expand_tab);
    EXPECT_EQ(editor.options.indent_width, 8);

    handle_command (
        commands,
        "option -r '/test/data/detect-indent\\.ini$' detect-indent 2,4,8;"
        "open test/data/detect-indent.ini"
    );

    EXPECT_EQ(buffer->options.detect_indent, 1 << 1 | 1 << 3 | 1 << 7);
    EXPECT_TRUE(buffer->options.expand_tab);
    EXPECT_EQ(buffer->options.indent_width, 2);

    handle_command(commands, "close");
}

static void test_handle_binding(void)
{
    handle_command(commands, "bind ^A 'insert zzz'; open");

    // Bound command should be cached
    const KeyBinding *binding = lookup_binding(MOD_CTRL | 'A');
    ASSERT_NONNULL(binding);
    EXPECT_PTREQ(binding->cmd, find_command(commands, "insert"));
    EXPECT_EQ(binding->a.nr_flags, 0);
    EXPECT_EQ(binding->a.nr_args, 1);
    EXPECT_STREQ(binding->a.args[0], "zzz");
    EXPECT_NULL(binding->a.args[1]);

    handle_binding(MOD_CTRL | 'A');
    const Block *block = BLOCK(buffer->blocks.next);
    ASSERT_NONNULL(block);
    ASSERT_EQ(block->size, 4);
    EXPECT_EQ(block->nl, 1);
    EXPECT_EQ(memcmp(block->data, "zzz\n", 4), 0);
    EXPECT_TRUE(undo());
    EXPECT_EQ(block->size, 0);
    EXPECT_EQ(block->nl, 0);
    EXPECT_FALSE(undo());
    handle_command(commands, "close");
}

static void test_regexp_match(void)
{
    static const char buf[] = "fn(a);\n";

    PointerArray a = PTR_ARRAY_INIT;
    bool matched = regexp_match("^[a-z]+\\(", buf, sizeof(buf) - 1, &a);
    EXPECT_TRUE(matched);
    EXPECT_EQ(a.count, 1);
    EXPECT_STREQ(a.ptrs[0], "fn(");
    ptr_array_free(&a);

    ptr_array_init(&a, 0);
    matched = regexp_match("^[a-z]+\\([0-9]", buf, sizeof(buf) - 1, &a);
    EXPECT_FALSE(matched);
    EXPECT_EQ(a.count, 0);
    ptr_array_free(&a);
}

static void test_posix_sanity(void)
{
    // This is not guaranteed by ISO C99, but it is required by POSIX
    // and is relied upon by this codebase:
    ASSERT_EQ(CHAR_BIT, 8);
}

int main(void)
{
    test_posix_sanity();
    init_editor_state();

    test_relative_filename();

    test_command();
    test_editorconfig();
    test_encoding();
    test_filetype();
    test_util();
    test_terminal();
    test_cmdline();
    test_regexp_match();
    test_syntax();

    init_headless_mode();
    test_exec_config();
    test_detect_indent();
    test_handle_binding();

    return failed ? 1 : 0;
}
