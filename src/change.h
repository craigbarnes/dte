#ifndef CHANGE_H
#define CHANGE_H

#include <stdbool.h>
#include <stddef.h>
#include "util/macros.h"
#include "view.h"

typedef enum {
    CHANGE_MERGE_NONE,
    CHANGE_MERGE_INSERT,
    CHANGE_MERGE_DELETE,
    CHANGE_MERGE_ERASE,
} ChangeMergeEnum;

typedef struct Change {
    struct Change *next;
    struct Change **prev;
    unsigned long nr_prev;
    bool move_after; // Move after inserted text when undoing delete?
    size_t offset;
    size_t del_count;
    size_t ins_count;
    char *buf; // Deleted bytes (inserted bytes need not be saved)
} Change;

void begin_change(ChangeMergeEnum m);
void end_change(void);
void begin_change_chain(void);
void end_change_chain(View *view) NONNULL_ARGS;
bool undo(View *view) NONNULL_ARGS WARN_UNUSED_RESULT;
bool redo(View *view, unsigned long change_id) NONNULL_ARGS WARN_UNUSED_RESULT;
void free_changes(Change *c) NONNULL_ARGS;
void buffer_insert_bytes(View *view, const char *buf, size_t len) NONNULL_ARG(1);
void buffer_delete_bytes(View *view, size_t len) NONNULL_ARGS;
void buffer_erase_bytes(View *view, size_t len) NONNULL_ARGS;
void buffer_replace_bytes(View *view, size_t del_count, const char *ins, size_t ins_count) NONNULL_ARG(1);

#endif
