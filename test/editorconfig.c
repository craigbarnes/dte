#include <stdlib.h>
#include "test.h"
#include "editorconfig/editorconfig.h"
#include "editorconfig/ini.h"
#include "editorconfig/match.h"
#include "options.h"
#include "util/bit.h"
#include "util/path.h"

static void test_ini_parse(TestContext *ctx)
{
    static const StringView input = STRING_VIEW (
        " \t  key = val   \n"
        "\n"
        " \t [section 1]  \n"
        "xyz = 123\n"
        "\tfoo bar = this;is#not#a;comment\n"
        "[section 2]\n"
        " x=0\n"
        "[]\n"
        " \t # comm=ent\n"
        "; comm=ent\n"
        "z=."
    );

    IniParser ini = {.input = input};
    EXPECT_TRUE(ini_parse(&ini));
    EXPECT_EQ(ini.pos, 17);
    EXPECT_EQ(ini.name_count, 1);
    EXPECT_STRVIEW_EQ_CSTRING(ini.section, "");
    EXPECT_STRVIEW_EQ_CSTRING(ini.name, "key");
    EXPECT_STRVIEW_EQ_CSTRING(ini.value, "val");

    EXPECT_TRUE(ini_parse(&ini));
    EXPECT_EQ(ini.pos, 45);
    EXPECT_EQ(ini.name_count, 1);
    EXPECT_STRVIEW_EQ_CSTRING(ini.section, "section 1");
    EXPECT_STRVIEW_EQ_CSTRING(ini.name, "xyz");
    EXPECT_STRVIEW_EQ_CSTRING(ini.value, "123");

    EXPECT_TRUE(ini_parse(&ini));
    EXPECT_EQ(ini.pos, 78);
    EXPECT_EQ(ini.name_count, 2);
    EXPECT_STRVIEW_EQ_CSTRING(ini.section, "section 1");
    EXPECT_STRVIEW_EQ_CSTRING(ini.name, "foo bar");
    EXPECT_STRVIEW_EQ_CSTRING(ini.value, "this;is#not#a;comment");

    EXPECT_TRUE(ini_parse(&ini));
    EXPECT_EQ(ini.pos, 95);
    EXPECT_EQ(ini.name_count, 1);
    EXPECT_STRVIEW_EQ_CSTRING(ini.section, "section 2");
    EXPECT_STRVIEW_EQ_CSTRING(ini.name, "x");
    EXPECT_STRVIEW_EQ_CSTRING(ini.value, "0");

    EXPECT_TRUE(ini_parse(&ini));
    EXPECT_EQ(ini.pos, 126);
    EXPECT_EQ(ini.name_count, 1);
    EXPECT_STRVIEW_EQ_CSTRING(ini.section, "");
    EXPECT_STRVIEW_EQ_CSTRING(ini.name, "z");
    EXPECT_STRVIEW_EQ_CSTRING(ini.value, ".");

    EXPECT_FALSE(ini_parse(&ini));
}

static void test_ec_pattern_to_regex(TestContext *ctx)
{
    const StringView dir = STRING_VIEW("/dir");

    String str = ec_pattern_to_regex(strview("\\[ab]"), dir);
    EXPECT_STRING_EQ_CSTRING(&str, "^/dir/(.*/)?\\[ab]$");
    string_free(&str);

    str = ec_pattern_to_regex(strview("**.c"), dir);
    EXPECT_STRING_EQ_CSTRING(&str, "^/dir/(.*/)?.*\\.c$");
    string_free(&str);

    str = ec_pattern_to_regex(strview("\\*\\*.c"), dir);
    EXPECT_STRING_EQ_CSTRING(&str, "^/dir/(.*/)?\\*\\*\\.c$");
    string_free(&str);

    str = ec_pattern_to_regex(strview("/xyz/\\[test-dir]/\\**.conf"), dir);
    EXPECT_STRING_EQ_CSTRING(&str, "^/dir/xyz/\\[test-dir]/\\*[^/]*\\.conf$");
    string_free(&str);
}

static void test_ec_pattern_match(TestContext *ctx)
{
    const StringView dir = STRING_VIEW("/dir");

    EXPECT_TRUE(ec_pattern_match(strview("*"), dir, "/dir/file.c"));
    EXPECT_FALSE(ec_pattern_match(strview("*"), dir, "/other-dir/file.c"));
    EXPECT_TRUE(ec_pattern_match(strview("*.{c,h}"), dir, "/dir/file.c"));
    EXPECT_TRUE(ec_pattern_match(strview("*.{foo}"), dir, "/dir/file.foo"));

    EXPECT_TRUE(ec_pattern_match(strview("*.{foo{bar,baz}}"), dir, "/dir/file.foobaz"));
    EXPECT_FALSE(ec_pattern_match(strview("*.{foo{bar,baz}}"), dir, "/dir/file.foo"));

    EXPECT_TRUE(ec_pattern_match(strview("a/**/b/c/*.[ch]"), dir, "/dir/a/zzz/yyy/foo/b/c/file.h"));
    EXPECT_FALSE(ec_pattern_match(strview("a/*/b/c/*.[ch]"), dir, "/dir/a/zzz/yyy/foo/b/c/file.h"));

    EXPECT_TRUE(ec_pattern_match(strview("}*.{x,y}"), dir, "/dir/}foo.y"));
    EXPECT_FALSE(ec_pattern_match(strview("}*.{x,y}"), dir, "/dir/foo.y"));
    EXPECT_TRUE(ec_pattern_match(strview("{}*.{x,y}"), dir, "/dir/foo.y"));

    EXPECT_TRUE(ec_pattern_match(strview("*.[xyz]"), dir, "/dir/foo.z"));
    EXPECT_FALSE(ec_pattern_match(strview("*.[xyz"), dir, "/dir/foo.z"));
    EXPECT_TRUE(ec_pattern_match(strview("*.[xyz"), dir, "/dir/foo.[xyz"));

    EXPECT_TRUE(ec_pattern_match(strview("*.[!xyz]"), dir, "/dir/foo.a"));
    EXPECT_FALSE(ec_pattern_match(strview("*.[!xyz]"), dir, "/dir/foo.z"));

    EXPECT_TRUE(ec_pattern_match(strview("*.["), dir, "/dir/foo.["));
    EXPECT_TRUE(ec_pattern_match(strview("*.[a"), dir, "/dir/foo.[a"));

    EXPECT_TRUE(ec_pattern_match(strview("*.[abc]def"), dir, "/dir/foo.bdef"));

    EXPECT_TRUE(ec_pattern_match(strview("x{{foo,},}"), dir, "/dir/x"));
    EXPECT_TRUE(ec_pattern_match(strview("x{{foo,},}"), dir, "/dir/xfoo"));

    EXPECT_TRUE(ec_pattern_match(strview("file.{,,x,,y,,}"), dir, "/dir/file.x"));
    EXPECT_TRUE(ec_pattern_match(strview("file.{,,x,,y,,}"), dir, "/dir/file."));
    EXPECT_FALSE(ec_pattern_match(strview("file.{,,x,,y,,}"), dir, "/dir/file.z"));

    EXPECT_TRUE(ec_pattern_match(strview("*.x,y,z"), dir, "/dir/file.x,y,z"));
    EXPECT_TRUE(ec_pattern_match(strview("*.{x,y,z}"), dir, "/dir/file.y"));
    EXPECT_FALSE(ec_pattern_match(strview("*.{x,y,z}"), dir, "/dir/file.x,y,z"));
    EXPECT_FALSE(ec_pattern_match(strview("*.{x,y,z}"), dir, "/dir/file.{x,y,z}"));

    EXPECT_TRUE(ec_pattern_match(strview("file.{{{a,b,{c,,d}}}}"), dir, "/dir/file.d"));
    EXPECT_TRUE(ec_pattern_match(strview("file.{{{a,b,{c,,d}}}}"), dir, "/dir/file."));
    EXPECT_FALSE(ec_pattern_match(strview("file.{{{a,b,{c,d}}}}"), dir, "/dir/file."));

    EXPECT_TRUE(ec_pattern_match(strview("file.{c[vl]d,inc}"), dir, "/dir/file.cvd"));
    EXPECT_TRUE(ec_pattern_match(strview("file.{c[vl]d,inc}"), dir, "/dir/file.cld"));
    EXPECT_TRUE(ec_pattern_match(strview("file.{c[vl]d,inc}"), dir, "/dir/file.inc"));
    EXPECT_FALSE(ec_pattern_match(strview("file.{c[vl]d,inc}"), dir, "/dir/file.cd"));

    EXPECT_TRUE(ec_pattern_match(strview("a?b.c"), dir, "/dir/a_b.c"));
    EXPECT_FALSE(ec_pattern_match(strview("a?b.c"), dir, "/dir/a/b.c"));

    EXPECT_TRUE(ec_pattern_match(strview("a\\[.abc"), dir, "/dir/a[.abc"));
    EXPECT_TRUE(ec_pattern_match(strview("a\\{.abc"), dir, "/dir/a{.abc"));
    EXPECT_TRUE(ec_pattern_match(strview("a\\*.abc"), dir, "/dir/a*.abc"));
    EXPECT_TRUE(ec_pattern_match(strview("a\\?.abc"), dir, "/dir/a?.abc"));
    EXPECT_FALSE(ec_pattern_match(strview("a\\*.abc"), dir, "/dir/az.abc"));
    EXPECT_FALSE(ec_pattern_match(strview("a\\?.abc"), dir, "/dir/az.abc"));

    EXPECT_TRUE(ec_pattern_match(strview("{{{a}}}"), dir, "/dir/a"));
    EXPECT_FALSE(ec_pattern_match(strview("{{{a}}"), dir, "/dir/a"));

    EXPECT_TRUE(ec_pattern_match(strview("/dir2/**.ext"), dir, "/dir/dir2/file.ext"));
    EXPECT_TRUE(ec_pattern_match(strview("/dir2/**.ext"), dir, "/dir/dir2/dir3/file.ext"));
    EXPECT_FALSE(ec_pattern_match(strview("/dir2/**.ext"), dir, "/x/dir2/dir3/file.ext"));

    // It's debatable whether this edge case behavior is sensible,
    // but it's tested here anyway for the sake of UBSan coverage
    EXPECT_TRUE(ec_pattern_match(strview("*.xyz\\"), dir, "/dir/file.xyz\\"));
    EXPECT_FALSE(ec_pattern_match(strview("*.xyz\\"), dir, "/dir/file.xyz"));
}

static void test_ec_options_struct(TestContext *ctx)
{
    const EditorConfigOptions opts = {
        .indent_size = INDENT_WIDTH_MAX,
        .tab_width = TAB_WIDTH_MAX,
        .max_line_length = TEXT_WIDTH_MAX,
        .indent_style = INDENT_STYLE_SPACE,
        .indent_size_is_tab = true,
    };

    // Ensure bitfield widths are sufficient for maximum values
    EXPECT_EQ(opts.indent_size, INDENT_WIDTH_MAX);
    EXPECT_EQ(opts.tab_width, TAB_WIDTH_MAX);
    EXPECT_EQ(opts.max_line_length, TEXT_WIDTH_MAX);
    EXPECT_EQ(opts.indent_style, INDENT_STYLE_SPACE);
    EXPECT_TRUE(opts.indent_size_is_tab);
    EXPECT_EQ(umax_bitwidth(INDENT_WIDTH_MAX), 4);
    EXPECT_EQ(umax_bitwidth(TAB_WIDTH_MAX), 4);
    EXPECT_EQ(umax_bitwidth(TEXT_WIDTH_MAX), 10);
    EXPECT_EQ(umax_bitwidth(INDENT_STYLE_SPACE), 2);
}

static void test_get_editorconfig_options(TestContext *ctx)
{
    char *path = path_absolute("test/data/file.0foo.z");
    ASSERT_NONNULL(path);
    EditorConfigOptions opts = get_editorconfig_options(path);
    free(path);
    EXPECT_EQ(opts.indent_style, INDENT_STYLE_SPACE);
    EXPECT_EQ(opts.indent_size, 3);
    EXPECT_EQ(opts.tab_width, 3);
    EXPECT_EQ(opts.max_line_length, 68);
    EXPECT_FALSE(opts.indent_size_is_tab);

    path = path_absolute("test/data/file.foo");
    ASSERT_NONNULL(path);
    opts = get_editorconfig_options(path);
    free(path);
    EXPECT_EQ(opts.indent_style, INDENT_STYLE_UNSPECIFIED);
    EXPECT_EQ(opts.indent_size, 0);
    EXPECT_EQ(opts.tab_width, 0);
    EXPECT_EQ(opts.max_line_length, 0);
    EXPECT_FALSE(opts.indent_size_is_tab);
}

static const TestEntry tests[] = {
    TEST(test_ini_parse),
    TEST(test_ec_pattern_to_regex),
    TEST(test_ec_pattern_match),
    TEST(test_ec_options_struct),
    TEST(test_get_editorconfig_options),
};

const TestGroup editorconfig_tests = TEST_GROUP(tests);
