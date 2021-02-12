#include <stdlib.h>
#include "test.h"
#include "editorconfig/editorconfig.h"
#include "editorconfig/match.h"
#include "util/path.h"

static void test_editorconfig_pattern_match(void)
{
    #define patmatch(s, f) (ec_pattern_match(s, STRLEN(s), f))

    EXPECT_TRUE(patmatch("*", "file.c"));
    EXPECT_TRUE(patmatch("*.{c,h}", "file.c"));
    EXPECT_TRUE(patmatch("*.{foo}", "file.foo"));

    EXPECT_TRUE(patmatch("*.{foo{bar,baz}}", "file.foobaz"));
    EXPECT_FALSE(patmatch("*.{foo{bar,baz}}", "file.foo"));

    EXPECT_TRUE(patmatch("a/**/b/c/*.[ch]", "a/zzz/yyy/foo/b/c/file.h"));
    EXPECT_FALSE(patmatch("a/*/b/c/*.[ch]", "a/zzz/yyy/foo/b/c/file.h"));

    EXPECT_TRUE(patmatch("}*.{x,y}", "}foo.y"));
    EXPECT_FALSE(patmatch("}*.{x,y}", "foo.y"));
    EXPECT_TRUE(patmatch("{}*.{x,y}", "foo.y"));

    EXPECT_TRUE(patmatch("*.[xyz]", "foo.z"));
    EXPECT_FALSE(patmatch("*.[xyz", "foo.z"));
    EXPECT_TRUE(patmatch("*.[xyz", "foo.[xyz"));

    EXPECT_TRUE(patmatch("*.[!xyz]", "foo.a"));
    EXPECT_FALSE(patmatch("*.[!xyz]", "foo.z"));

    EXPECT_TRUE(patmatch("*.[", "foo.["));
    EXPECT_TRUE(patmatch("*.[a", "foo.[a"));

    EXPECT_TRUE(patmatch("*.[abc]def", "foo.bdef"));

    EXPECT_TRUE(patmatch("x{{foo,},}", "x"));
    EXPECT_TRUE(patmatch("x{{foo,},}", "xfoo"));

    EXPECT_TRUE(patmatch("file.{,,x,,y,,}", "file.x"));
    EXPECT_TRUE(patmatch("file.{,,x,,y,,}", "file."));
    EXPECT_FALSE(patmatch("file.{,,x,,y,,}", "file.z"));

    EXPECT_TRUE(patmatch("*.x,y,z", "file.x,y,z"));
    EXPECT_TRUE(patmatch("*.{x,y,z}", "file.y"));
    EXPECT_FALSE(patmatch("*.{x,y,z}", "file.x,y,z"));
    EXPECT_FALSE(patmatch("*.{x,y,z}", "file.{x,y,z}"));

    EXPECT_TRUE(patmatch("file.{{{a,b,{c,,d}}}}", "file.d"));
    EXPECT_TRUE(patmatch("file.{{{a,b,{c,,d}}}}", "file."));
    EXPECT_FALSE(patmatch("file.{{{a,b,{c,d}}}}", "file."));

    EXPECT_TRUE(patmatch("file.{c[vl]d,inc}", "file.cvd"));
    EXPECT_TRUE(patmatch("file.{c[vl]d,inc}", "file.cld"));
    EXPECT_TRUE(patmatch("file.{c[vl]d,inc}", "file.inc"));
    EXPECT_FALSE(patmatch("file.{c[vl]d,inc}", "file.cd"));

    EXPECT_TRUE(patmatch("a?b.c", "a_b.c"));
    EXPECT_FALSE(patmatch("a?b.c", "a/b.c"));

    EXPECT_TRUE(patmatch("a\\[.abc", "a[.abc"));
    EXPECT_TRUE(patmatch("a\\{.abc", "a{.abc"));
    EXPECT_TRUE(patmatch("a\\*.abc", "a*.abc"));
    EXPECT_TRUE(patmatch("a\\?.abc", "a?.abc"));
    EXPECT_FALSE(patmatch("a\\*.abc", "az.abc"));
    EXPECT_FALSE(patmatch("a\\?.abc", "az.abc"));

    EXPECT_TRUE(patmatch("{{{a}}}", "a"));
    EXPECT_FALSE(patmatch("{{{a}}", "a"));

    // It's debatable whether this edge case behavior is sensible,
    // but it's tested here anyway for the sake of UBSan coverage
    EXPECT_TRUE(patmatch("*.xyz\\", "file.xyz\\"));
    EXPECT_FALSE(patmatch("*.xyz\\", "file.xyz"));

    #undef patmatch
}

static void test_get_editorconfig_options(void)
{
    EditorConfigOptions opts = editorconfig_options_init();
    char *path = path_absolute("test/data/file.0foo.z");
    ASSERT_NONNULL(path);
    EXPECT_EQ(get_editorconfig_options(path, &opts), 0);
    free(path);
    EXPECT_EQ(opts.indent_style, INDENT_STYLE_SPACE);
    EXPECT_EQ(opts.indent_size, 3);
    EXPECT_EQ(opts.tab_width, 3);
    EXPECT_EQ(opts.max_line_length, 68);
    EXPECT_FALSE(opts.indent_size_is_tab);

    path = path_absolute("test/data/file.foo");
    ASSERT_NONNULL(path);
    EXPECT_EQ(get_editorconfig_options(path, &opts), 0);
    free(path);
    EXPECT_EQ(opts.indent_style, INDENT_STYLE_UNSPECIFIED);
    EXPECT_EQ(opts.indent_size, 0);
    EXPECT_EQ(opts.tab_width, 0);
    EXPECT_EQ(opts.max_line_length, 0);
    EXPECT_FALSE(opts.indent_size_is_tab);
}

static const TestEntry tests[] = {
    TEST(test_editorconfig_pattern_match),
    TEST(test_get_editorconfig_options),
};

const TestGroup editorconfig_tests = TEST_GROUP(tests);
