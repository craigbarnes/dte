#ifndef HISTORY_H
#define HISTORY_H

#include <stdbool.h>
#include <stddef.h>
#include "error.h"
#include "util/hashmap.h"
#include "util/macros.h"
#include "util/string.h"

typedef struct HistoryEntry {
    struct HistoryEntry *next;
    struct HistoryEntry *prev;
    char *text;
} HistoryEntry;

// This is a HashMap with a doubly-linked list running through the
// entries, in a way similar to the Java LinkedHashMap class. The
// HashMap allows duplicates to be found and re-inserted at the end
// of the list in O(1) time and the doubly-linked entries allow
// insertion-ordered traversal.
typedef struct {
    char *filename;
    HashMap entries;
    HistoryEntry *first;
    HistoryEntry *last;
    size_t max_entries;
} History;

void history_append(History *history, const char *text) NONNULL_ARGS;
bool history_search_forward(const History *history, const HistoryEntry **pos, const char *text) NONNULL_ARG(1, 3) WARN_UNUSED_RESULT;
bool history_search_backward(const History *history, const HistoryEntry **pos, const char *text) NONNULL_ARG(1, 3) WARN_UNUSED_RESULT;
void history_load(History *history, ErrorBuffer *ebuf, char *filename, size_t size_limit) NONNULL_ARGS;
void history_save(const History *history, ErrorBuffer *ebuf) NONNULL_ARGS;
void history_free(History *history) NONNULL_ARGS;
String history_dump(const History *history) NONNULL_ARGS;

#endif
