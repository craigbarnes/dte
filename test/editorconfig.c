#include <stdlib.h>
#include "test.h"
#include "../src/editorconfig/editorconfig.h"
#include "../src/editorconfig/match.h"
#include "../src/util/path.h"

static void test_editorconfig_pattern_match(void)
{
    EXPECT_TRUE(ec_pattern_match("*", "file.c"));
    EXPECT_TRUE(ec_pattern_match("*.{c,h}", "file.c"));
    EXPECT_TRUE(ec_pattern_match("*.{foo}", "file.foo"));

    EXPECT_TRUE(ec_pattern_match("*.{foo{bar,baz}}", "file.foobaz"));
    EXPECT_FALSE(ec_pattern_match("*.{foo{bar,baz}}", "file.foo"));

    EXPECT_TRUE(ec_pattern_match("a/**/b/c/*.[ch]", "a/zzz/yyy/foo/b/c/file.h"));
    EXPECT_FALSE(ec_pattern_match("a/*/b/c/*.[ch]", "a/zzz/yyy/foo/b/c/file.h"));

    EXPECT_TRUE(ec_pattern_match("}*.{x,y}", "}foo.y"));
    EXPECT_FALSE(ec_pattern_match("}*.{x,y}", "foo.y"));
    EXPECT_TRUE(ec_pattern_match("{}*.{x,y}", "foo.y"));

    EXPECT_TRUE(ec_pattern_match("*.[xyz]", "foo.z"));
    EXPECT_FALSE(ec_pattern_match("*.[xyz", "foo.z"));
    EXPECT_TRUE(ec_pattern_match("*.[xyz", "foo.[xyz"));

    EXPECT_TRUE(ec_pattern_match("*.[!xyz]", "foo.a"));
    EXPECT_FALSE(ec_pattern_match("*.[!xyz]", "foo.z"));

    EXPECT_TRUE(ec_pattern_match("*.[", "foo.["));
    EXPECT_TRUE(ec_pattern_match("*.[a", "foo.[a"));

    EXPECT_TRUE(ec_pattern_match("*.[abc]def", "foo.bdef"));

    EXPECT_TRUE(ec_pattern_match("x{{foo,},}", "x"));
    EXPECT_TRUE(ec_pattern_match("x{{foo,},}", "xfoo"));

    EXPECT_TRUE(ec_pattern_match("file.{,,x,,y,,}", "file.x"));
    EXPECT_TRUE(ec_pattern_match("file.{,,x,,y,,}", "file."));
    EXPECT_FALSE(ec_pattern_match("file.{,,x,,y,,}", "file.z"));

    EXPECT_TRUE(ec_pattern_match("file.{{{a,b,{c,,d}}}}", "file.d"));
    EXPECT_TRUE(ec_pattern_match("file.{{{a,b,{c,,d}}}}", "file."));
    EXPECT_FALSE(ec_pattern_match("file.{{{a,b,{c,d}}}}", "file."));
}

static void test_get_editorconfig_options(void)
{
    EditorConfigOptions opts;
    char *path = path_absolute("test/data/file.0foo.z");
    get_editorconfig_options(path, &opts);
    free(path);
    EXPECT_EQ(opts.indent_style, INDENT_STYLE_SPACE);
    EXPECT_EQ(opts.indent_size, 3);
    EXPECT_EQ(opts.tab_width, 3);
    EXPECT_EQ(opts.max_line_length, 68);
    EXPECT_FALSE(opts.indent_size_is_tab);

    path = path_absolute("test/data/file.foo");
    get_editorconfig_options(path, &opts);
    free(path);
    EXPECT_EQ(opts.indent_style, INDENT_STYLE_UNSPECIFIED);
    EXPECT_EQ(opts.indent_size, 0);
    EXPECT_EQ(opts.tab_width, 0);
    EXPECT_EQ(opts.max_line_length, 0);
    EXPECT_FALSE(opts.indent_size_is_tab);
}

void test_editorconfig(void)
{
    test_editorconfig_pattern_match();
    test_get_editorconfig_options();
}
