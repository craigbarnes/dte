#include "test.h"
#include "ctags.h"
#include "util/readfile.h"

static void test_parse_ctags_line(TestContext *ctx)
{
    const char *line = "foo\tfile.c\t/^int foo(char *s)$/;\"\tf\tfile:";
    Tag tag = {.pattern = NULL};
    EXPECT_TRUE(parse_ctags_line(&tag, line, strlen(line)));
    EXPECT_EQ(tag.name.length, 3);
    EXPECT_PTREQ(tag.name.data, line);
    EXPECT_EQ(tag.filename.length, 6);
    EXPECT_PTREQ(tag.filename.data, line + 4);
    EXPECT_STREQ(tag.pattern, "^int foo(char \\*s)$");
    EXPECT_EQ(tag.lineno, 0);
    EXPECT_EQ(tag.kind, 'f');
    EXPECT_EQ(tag.local, true);
    free_tag(&tag);

    line = "abc\t123.py\t/^backslashes \\\\ in \\ \\: pattern$/";
    tag = (Tag){.pattern = NULL};
    EXPECT_TRUE(parse_ctags_line(&tag, line, strlen(line)));
    EXPECT_EQ(tag.filename.length, 6);
    EXPECT_STREQ(tag.pattern, "^backslashes \\\\ in  : pattern$");
    EXPECT_EQ(tag.lineno, 0);
    EXPECT_EQ(tag.kind, 0);
    free_tag(&tag);

    line = "example\tsrc/xyz.c\t488;\"\tk";
    tag = (Tag){.pattern = NULL};
    EXPECT_TRUE(parse_ctags_line(&tag, line, strlen(line)));
    EXPECT_EQ(tag.filename.length, 9);
    EXPECT_NULL(tag.pattern);
    EXPECT_EQ(tag.lineno, 488);
    EXPECT_EQ(tag.kind, 'k');
    free_tag(&tag);

    line = "x\tstr.c\t12495\tz";
    tag = (Tag){.pattern = NULL};
    EXPECT_TRUE(parse_ctags_line(&tag, line, strlen(line)));
    EXPECT_NULL(tag.pattern);
    EXPECT_EQ(tag.lineno, 12495);
    EXPECT_EQ(tag.kind, 'z');
    free_tag(&tag);

    line = "name\tfile.c\t/^char after pattern with no tab delimiter/t";
    tag = (Tag){.pattern = NULL};
    EXPECT_FALSE(parse_ctags_line(&tag, line, strlen(line)));
    free_tag(&tag);

    line = "bar\tsource.c\t/^unterminated pattern\tf";
    EXPECT_FALSE(parse_ctags_line(&tag, line, strlen(line)));
    free_tag(&tag);

    StringView sv = STRING_VIEW("tag-name\tfile.txt\t/^embedded NUL \0 char/\tf");
    EXPECT_FALSE(parse_ctags_line(&tag, sv.data, sv.length));
    free_tag(&tag);
}

static void test_next_tag(TestContext *ctx)
{
    static const struct {
        const char *name;
        char kind;
        bool local;
    } expected[] = {
        {"MIN_SIZE", 'e', true},
        {"TOMBSTONE", 'e', true},
        {"hashmap_clear", 'f', false},
        {"hashmap_do_init", 'f', true},
        {"hashmap_do_insert", 'f', true},
        {"hashmap_find", 'f', false},
        {"hashmap_free", 'f', false},
        {"hashmap_init", 'f', false},
        {"hashmap_insert", 'f', false},
        {"hashmap_insert_or_replace", 'f', false},
        {"hashmap_remove", 'f', false},
        {"hashmap_resize", 'f', true},
    };

    char *buf;
    ssize_t len = read_file("test/data/ctags.txt", &buf, 8192);
    ASSERT_TRUE(len >= 64);

    StringView prefix = STRING_VIEW("");
    Tag t;
    for (size_t i = 0, pos = 0; next_tag(buf, len, &pos, &prefix, false, &t); i++) {
        EXPECT_STRVIEW_EQ_CSTRING(&t.name, expected[i].name);
        IEXPECT_EQ(t.kind, expected[i].kind);
        IEXPECT_EQ(t.local, expected[i].local);
        EXPECT_STRVIEW_EQ_CSTRING(&t.filename, "src/util/hashmap.c");
        IEXPECT_EQ(t.lineno, 0);
        free_tag(&t);
    }

    size_t pos = 0;
    t.name = string_view(NULL, 0);
    prefix = strview_from_cstring("hashmap_res");
    EXPECT_TRUE(next_tag(buf, len, &pos, &prefix, false, &t));
    EXPECT_STRVIEW_EQ_CSTRING(&t.name, "hashmap_resize");
    free_tag(&t);
    EXPECT_FALSE(next_tag(buf, len, &pos, &prefix, false, &t));
    pos = 0;
    EXPECT_FALSE(next_tag(buf, len, &pos, &prefix, true, &t));

    free(buf);
}

static const TestEntry tests[] = {
    TEST(test_parse_ctags_line),
    TEST(test_next_tag),
};

const TestGroup ctags_tests = TEST_GROUP(tests);
