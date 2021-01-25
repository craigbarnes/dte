#include <stdlib.h>
#include "test.h"
#include "history.h"

DISABLE_WARNING("-Wmissing-prototypes")

void test_history(void)
{
    History h = {.max_entries = 8};
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

    hashmap_free(&h.entries, free);
}
