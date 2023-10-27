#include <stdlib.h>
#include "test.h"
#include "file-history.h"
#include "history.h"
#include "util/numtostr.h"
#include "util/readfile.h"
#include "util/xmalloc.h"

static void test_history_append(TestContext *ctx)
{
    History h = {.max_entries = 7};
    history_append(&h, "A");
    EXPECT_EQ(h.entries.count, 1);
    EXPECT_STREQ(h.first->text, "A");
    EXPECT_NULL(h.first->prev);
    EXPECT_NULL(h.first->next);
    EXPECT_PTREQ(h.first, h.last);

    history_append(&h, "A");
    EXPECT_EQ(h.entries.count, 1);
    EXPECT_PTREQ(h.first, h.last);

    history_append(&h, "B");
    EXPECT_EQ(h.entries.count, 2);
    EXPECT_STREQ(h.first->text, "A");
    EXPECT_STREQ(h.last->text, "B");
    EXPECT_NULL(h.first->prev);
    EXPECT_NULL(h.last->next);
    EXPECT_PTREQ(h.first->next, h.last);
    EXPECT_PTREQ(h.last->prev, h.first);

    history_append(&h, "C");
    EXPECT_EQ(h.entries.count, 3);
    EXPECT_STREQ(h.first->text, "A");
    EXPECT_STREQ(h.first->next->text, "B");
    EXPECT_STREQ(h.last->prev->text, "B");
    EXPECT_STREQ(h.last->text, "C");
    EXPECT_NULL(h.first->prev);
    EXPECT_NULL(h.last->next);

    history_append(&h, "A");
    EXPECT_EQ(h.entries.count, 3);
    EXPECT_STREQ(h.last->text, "A");
    EXPECT_STREQ(h.first->text, "B");
    EXPECT_NULL(h.first->prev);
    EXPECT_NULL(h.last->next);
    EXPECT_STREQ(h.first->next->text, "C");
    EXPECT_STREQ(h.last->prev->text, "C");

    history_append(&h, "C");
    EXPECT_EQ(h.entries.count, 3);
    EXPECT_STREQ(h.last->text, "C");
    EXPECT_STREQ(h.first->text, "B");
    EXPECT_NULL(h.first->prev);
    EXPECT_NULL(h.last->next);
    EXPECT_STREQ(h.first->next->text, "A");
    EXPECT_STREQ(h.last->prev->text, "A");

    history_append(&h, "B");
    history_append(&h, "C");
    EXPECT_EQ(h.entries.count, 3);
    EXPECT_STREQ(h.first->text, "A");
    EXPECT_STREQ(h.first->next->text, "B");
    EXPECT_STREQ(h.last->prev->text, "B");
    EXPECT_STREQ(h.last->text, "C");

    history_append(&h, "D");
    history_append(&h, "E");
    history_append(&h, "F");
    history_append(&h, "G");
    EXPECT_EQ(h.entries.count, 7);
    EXPECT_STREQ(h.last->text, "G");
    EXPECT_STREQ(h.first->text, "A");
    EXPECT_STREQ(h.last->prev->text, "F");

    history_append(&h, "H");
    EXPECT_EQ(h.entries.count, 7);
    EXPECT_STREQ(h.last->text, "H");
    EXPECT_STREQ(h.first->text, "B");
    EXPECT_STREQ(h.last->prev->text, "G");

    history_append(&h, "I");
    EXPECT_EQ(h.entries.count, 7);
    EXPECT_STREQ(h.last->text, "I");
    EXPECT_STREQ(h.first->text, "C");
    EXPECT_STREQ(h.last->prev->text, "H");

    history_free(&h);
    h = (History){.max_entries = 2};
    EXPECT_EQ(h.entries.count, 0);

    history_append(&h, "1");
    EXPECT_EQ(h.entries.count, 1);
    EXPECT_STREQ(h.last->text, "1");
    EXPECT_STREQ(h.first->text, "1");

    history_append(&h, "2");
    EXPECT_EQ(h.entries.count, 2);
    EXPECT_STREQ(h.last->text, "2");
    EXPECT_STREQ(h.first->text, "1");

    history_append(&h, "3");
    EXPECT_EQ(h.entries.count, 2);
    EXPECT_STREQ(h.last->text, "3");
    EXPECT_STREQ(h.first->text, "2");

    history_free(&h);
}

static void test_history_search(TestContext *ctx)
{
    const char *filename = "test/data/history";
    History h = {.max_entries = 64};
    history_load(&h, xstrdup(filename), 4096);
    EXPECT_EQ(h.entries.count, 3);
    EXPECT_STREQ(h.filename, filename);
    ASSERT_NONNULL(h.first);
    EXPECT_STREQ(h.first->text, "one");
    ASSERT_NONNULL(h.last);
    EXPECT_STREQ(h.last->text, "three");
    ASSERT_NONNULL(h.first->next);
    EXPECT_STREQ(h.first->next->text, "two");
    ASSERT_NONNULL(h.last->prev);
    EXPECT_STREQ(h.last->prev->text, "two");
    EXPECT_NULL(h.first->prev);
    EXPECT_NULL(h.last->next);

    const HistoryEntry *e = h.last;
    EXPECT_STREQ(e->text, "three");
    EXPECT_TRUE(history_search_forward(&h, &e, ""));
    EXPECT_STREQ(e->text, "two");
    EXPECT_TRUE(history_search_forward(&h, &e, ""));
    EXPECT_STREQ(e->text, "one");
    EXPECT_FALSE(history_search_forward(&h, &e, ""));

    EXPECT_STREQ(e->text, "one");
    EXPECT_TRUE(history_search_backward(&h, &e, ""));
    EXPECT_STREQ(e->text, "two");
    EXPECT_TRUE(history_search_backward(&h, &e, ""));
    EXPECT_STREQ(e->text, "three");
    EXPECT_FALSE(history_search_backward(&h, &e, ""));

    EXPECT_STREQ(e->text, "three");
    EXPECT_TRUE(history_search_forward(&h, &e, "o"));
    EXPECT_STREQ(e->text, "one");
    EXPECT_TRUE(history_search_backward(&h, &e, "th"));
    EXPECT_STREQ(e->text, "three");

    free(h.filename);
    filename = "build/test/saved_history";
    h.filename = xstrdup(filename);
    history_save(&h);
    history_free(&h);
    char *buf = NULL;
    ssize_t n = read_file(filename, &buf);
    EXPECT_EQ(n, 14);
    EXPECT_STREQ(buf, "one\ntwo\nthree\n");
    free(buf);
}

// This test is done to ensure the HashMap can handle the constant
// churn from insertions and removals (i.e. that it rehashes the
// table to clean out tombstones, even when the number of real
// entries stops growing).
static void test_history_tombstone_pressure(TestContext *ctx)
{
    History h = {.max_entries = 512};
    for (unsigned int i = 0; i < 12000; i++) {
        history_append(&h, uint_to_str(i));
    }

    EXPECT_EQ(h.entries.count, h.max_entries);
    EXPECT_TRUE(h.entries.mask + 1 <= 2048);
    history_free(&h);
}

static void test_file_history_find(TestContext *ctx)
{
    const char fh_filename[] = "test/data/file-history";
    FileHistory h = {.filename = NULL};
    file_history_load(&h, xstrdup(fh_filename), 4096);
    EXPECT_STREQ(h.filename, fh_filename);
    EXPECT_EQ(h.entries.count, 3);

    const FileHistoryEntry *first = h.first;
    ASSERT_NONNULL(first);
    ASSERT_NONNULL(first->next);
    EXPECT_NULL(first->prev);
    EXPECT_STREQ(first->filename, "/etc/hosts");
    EXPECT_EQ(first->row, 3);
    EXPECT_EQ(first->col, 42);

    const FileHistoryEntry *last = h.last;
    ASSERT_NONNULL(last);
    ASSERT_NONNULL(last->prev);
    EXPECT_NULL(last->next);
    EXPECT_STREQ(last->filename, "/home/user/file.txt");
    EXPECT_EQ(last->row, 4521);
    EXPECT_EQ(last->col, 1);

    const FileHistoryEntry *mid = first->next;
    ASSERT_NONNULL(mid);
    EXPECT_PTREQ(mid, last->prev);
    EXPECT_PTREQ(mid->prev, first);
    EXPECT_PTREQ(mid->next, last);
    EXPECT_STREQ(mid->filename, "/tmp/foo");
    EXPECT_EQ(mid->row, 123);
    EXPECT_EQ(mid->col, 456);

    unsigned long row = 0, col = 0;
    EXPECT_TRUE(file_history_find(&h, first->filename, &row, &col));
    EXPECT_EQ(row, first->row);
    EXPECT_EQ(col, first->col);
    EXPECT_TRUE(file_history_find(&h, mid->filename, &row, &col));
    EXPECT_EQ(row, mid->row);
    EXPECT_EQ(col, mid->col);
    EXPECT_TRUE(file_history_find(&h, last->filename, &row, &col));
    EXPECT_EQ(row, last->row);
    EXPECT_EQ(col, last->col);

    row = col = 99;
    EXPECT_FALSE(file_history_find(&h, "/tmp/_", &row, &col));
    EXPECT_EQ(row, 99);
    EXPECT_EQ(col, 99);

    file_history_free(&h);
}

static const TestEntry tests[] = {
    TEST(test_history_append),
    TEST(test_history_search),
    TEST(test_history_tombstone_pressure),
    TEST(test_file_history_find),
};

const TestGroup history_tests = TEST_GROUP(tests);
