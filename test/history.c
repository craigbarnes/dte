#include <stdlib.h>
#include "test.h"
#include "history.h"
#include "util/numtostr.h"
#include "util/readfile.h"

static void test_history_add(TestContext *ctx)
{
    History h = {.max_entries = 7};
    history_add(&h, "A");
    EXPECT_EQ(h.entries.count, 1);
    EXPECT_STREQ(h.first->text, "A");
    EXPECT_NULL(h.first->prev);
    EXPECT_NULL(h.first->next);
    EXPECT_PTREQ(h.first, h.last);

    history_add(&h, "A");
    EXPECT_EQ(h.entries.count, 1);
    EXPECT_PTREQ(h.first, h.last);

    history_add(&h, "B");
    EXPECT_EQ(h.entries.count, 2);
    EXPECT_STREQ(h.first->text, "A");
    EXPECT_STREQ(h.last->text, "B");
    EXPECT_NULL(h.first->prev);
    EXPECT_NULL(h.last->next);
    EXPECT_PTREQ(h.first->next, h.last);
    EXPECT_PTREQ(h.last->prev, h.first);

    history_add(&h, "C");
    EXPECT_EQ(h.entries.count, 3);
    EXPECT_STREQ(h.first->text, "A");
    EXPECT_STREQ(h.first->next->text, "B");
    EXPECT_STREQ(h.last->prev->text, "B");
    EXPECT_STREQ(h.last->text, "C");
    EXPECT_NULL(h.first->prev);
    EXPECT_NULL(h.last->next);

    history_add(&h, "A");
    EXPECT_EQ(h.entries.count, 3);
    EXPECT_STREQ(h.last->text, "A");
    EXPECT_STREQ(h.first->text, "B");
    EXPECT_NULL(h.first->prev);
    EXPECT_NULL(h.last->next);
    EXPECT_STREQ(h.first->next->text, "C");
    EXPECT_STREQ(h.last->prev->text, "C");

    history_add(&h, "C");
    EXPECT_EQ(h.entries.count, 3);
    EXPECT_STREQ(h.last->text, "C");
    EXPECT_STREQ(h.first->text, "B");
    EXPECT_NULL(h.first->prev);
    EXPECT_NULL(h.last->next);
    EXPECT_STREQ(h.first->next->text, "A");
    EXPECT_STREQ(h.last->prev->text, "A");

    history_add(&h, "B");
    history_add(&h, "C");
    EXPECT_EQ(h.entries.count, 3);
    EXPECT_STREQ(h.first->text, "A");
    EXPECT_STREQ(h.first->next->text, "B");
    EXPECT_STREQ(h.last->prev->text, "B");
    EXPECT_STREQ(h.last->text, "C");

    history_add(&h, "D");
    history_add(&h, "E");
    history_add(&h, "F");
    history_add(&h, "G");
    EXPECT_EQ(h.entries.count, 7);
    EXPECT_STREQ(h.last->text, "G");
    EXPECT_STREQ(h.first->text, "A");
    EXPECT_STREQ(h.last->prev->text, "F");

    history_add(&h, "H");
    EXPECT_EQ(h.entries.count, 7);
    EXPECT_STREQ(h.last->text, "H");
    EXPECT_STREQ(h.first->text, "B");
    EXPECT_STREQ(h.last->prev->text, "G");

    history_add(&h, "I");
    EXPECT_EQ(h.entries.count, 7);
    EXPECT_STREQ(h.last->text, "I");
    EXPECT_STREQ(h.first->text, "C");
    EXPECT_STREQ(h.last->prev->text, "H");

    history_free(&h);
    h = (History){.max_entries = 2};
    EXPECT_EQ(h.entries.count, 0);

    history_add(&h, "1");
    EXPECT_EQ(h.entries.count, 1);
    EXPECT_STREQ(h.last->text, "1");
    EXPECT_STREQ(h.first->text, "1");

    history_add(&h, "2");
    EXPECT_EQ(h.entries.count, 2);
    EXPECT_STREQ(h.last->text, "2");
    EXPECT_STREQ(h.first->text, "1");

    history_add(&h, "3");
    EXPECT_EQ(h.entries.count, 2);
    EXPECT_STREQ(h.last->text, "3");
    EXPECT_STREQ(h.first->text, "2");

    history_free(&h);
}

static void test_history_search(TestContext *ctx)
{
    History h = {.max_entries = 64};
    history_load(&h, "test/data/history");
    EXPECT_EQ(h.entries.count, 3);
    EXPECT_STREQ(h.first->text, "one");
    EXPECT_STREQ(h.last->text, "three");
    EXPECT_STREQ(h.first->next->text, "two");
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

    h.filename = "build/test/saved_history";
    history_save(&h);
    history_free(&h);
    char *buf = NULL;
    ssize_t n = read_file(h.filename, &buf);
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
        history_add(&h, uint_to_str(i));
    }

    EXPECT_EQ(h.entries.count, h.max_entries);
    EXPECT_TRUE(h.entries.mask + 1 <= 2048);
    history_free(&h);
}

static const TestEntry tests[] = {
    TEST(test_history_add),
    TEST(test_history_search),
    TEST(test_history_tombstone_pressure),
};

const TestGroup history_tests = TEST_GROUP(tests);
