#include <sys/stat.h>
#include "test.h"
#include "ctags.h"
#include "util/ptr-array.h"
#include "util/readfile.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"

static void test_next_tag(void)
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

    const char path[] = "test/data/ctags.txt";
    char *buf;
    ssize_t size = read_file(path, &buf);
    ASSERT_TRUE(size >= 64);

    TagFile tf = {
        .filename = xstrdup(path),
        .buf = buf,
        .size = size,
    };

    Tag t;
    for (size_t i = 0, pos = 0; next_tag(&tf, &pos, "", false, &t); i++) {
        IEXPECT_TRUE(strview_equal_cstring(&t.name, expected[i].name));
        IEXPECT_EQ(t.kind, expected[i].kind);
        IEXPECT_EQ(t.local, expected[i].local);
        IEXPECT_TRUE(strview_equal_cstring(&t.filename, "src/util/hashmap.c"));
        IEXPECT_EQ(t.lineno, 0);
        free_tag(&t);
    }

    size_t pos = 0;
    t.name = string_view(NULL, 0);
    EXPECT_TRUE(next_tag(&tf, &pos, "hashmap_res", false, &t));
    EXPECT_TRUE(strview_equal_cstring(&t.name, "hashmap_resize"));
    free_tag(&t);
    EXPECT_FALSE(next_tag(&tf, &pos, "hashmap_res", false, &t));
    pos = 0;
    EXPECT_FALSE(next_tag(&tf, &pos, "hashmap_res", true, &t));

    free(tf.filename);
    free(tf.buf);
}

static const TestEntry tests[] = {
    TEST(test_next_tag),
};

const TestGroup ctags_tests = TEST_GROUP(tests);
