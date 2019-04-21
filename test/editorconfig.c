#include "test.h"
#include "../src/editorconfig/match.h"

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
}

void test_editorconfig(void)
{
    test_editorconfig_pattern_match();
}
