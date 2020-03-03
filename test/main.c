#include <langinfo.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "../src/bind.h"
#include "../src/block.h"
#include "../src/buffer.h"
#include "../src/command.h"
#include "../src/editor.h"
#include "../src/regexp.h"
#include "../src/util/path.h"
#include "../src/util/str-util.h"
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
void test_config(void);

static void test_handle_binding(void)
{
    handle_command(&commands, "bind ^A 'insert zzz'; open", false);

    // Bound command should be cached
    const KeyBinding *binding = lookup_binding(MOD_CTRL | 'A');
    const Command *insert = find_normal_command("insert");
    ASSERT_NONNULL(binding);
    ASSERT_NONNULL(insert);
    EXPECT_PTREQ(binding->cmd, insert->cmd);
    EXPECT_EQ(binding->a.nr_flags, 0);
    EXPECT_EQ(binding->a.nr_args, 1);
    EXPECT_STREQ(binding->a.args[0], "zzz");
    EXPECT_NULL(binding->a.args[1]);

    handle_binding(MOD_CTRL | 'A');
    const Block *block = BLOCK(buffer->blocks.next);
    ASSERT_NONNULL(block);
    ASSERT_EQ(block->size, 4);
    EXPECT_EQ(block->nl, 1);
    EXPECT_TRUE(mem_equal(block->data, "zzz\n", 4));
    EXPECT_TRUE(undo());
    EXPECT_EQ(block->size, 0);
    EXPECT_EQ(block->nl, 0);
    EXPECT_FALSE(undo());
    handle_command(&commands, "close", false);
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
    ASSERT_TRUE(sizeof(int) >= 4);

    IGNORE_WARNING("-Wformat-truncation")

    // Some snprintf(3) implementations historically returned -1 in case of
    // truncation. C99 and POSIX 2001 both require that it return the full
    // size of the formatted string, as if there had been enough space.
    char buf[8] = "........";
    ASSERT_EQ(snprintf(buf, 8, "0123456789"), 10);
    ASSERT_EQ(buf[7], '\0');
    EXPECT_STREQ(buf, "0123456");

    // C99 and POSIX 2001 also require the same behavior as above when the
    // size argument is 0 (and allow the buffer argument to be NULL).
    ASSERT_EQ(snprintf(NULL, 0, "987654321"), 9);
    EXPECT_STREQ(buf, "0123456");

    UNIGNORE_WARNINGS
}

int main(void)
{
    test_posix_sanity();
    init_editor_state();

    const char *ver = getenv("DTE_VERSION");
    EXPECT_NONNULL(ver);
    EXPECT_STREQ(ver, editor.version);

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
    test_config();
    test_handle_binding();

    return failed ? 1 : 0;
}
