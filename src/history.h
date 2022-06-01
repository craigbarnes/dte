#ifndef HISTORY_H
#define HISTORY_H

#include <stdbool.h>
#include <stddef.h>
#include "util/hashmap.h"
#include "util/macros.h"

typedef struct HistoryEntry {
    struct HistoryEntry *next;
    struct HistoryEntry *prev;
    char *text;
} HistoryEntry;

// This is a HashMap with a doubly-linked list running through the
// entries, in a way similar to the Java LinkedHashMap class. The
// HashMap allows duplicates to be found and re-inserted at the end
// of the list in O(1) time and the doubly-linked entries allow
// ordered traversal.
typedef struct {
    char *filename;
    HashMap entries;
    HistoryEntry *first;
    HistoryEntry *last;
    size_t max_entries;
} History;

void history_add(History *history, const char *text);
bool history_search_forward(const History *history, const HistoryEntry **pos, const char *text) WARN_UNUSED_RESULT;
bool history_search_backward(const History *history, const HistoryEntry **pos, const char *text) WARN_UNUSED_RESULT;
void history_load(History *history, char *filename);
void history_save(const History *history);
void history_free(History *history);

#endif
