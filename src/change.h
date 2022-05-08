#ifndef CHANGE_H
#define CHANGE_H

#include <stdbool.h>
#include <stddef.h>
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
void end_change_chain(View *v);
bool undo(View *v);
bool redo(View *v, unsigned long change_id);
void free_changes(Change *c);
void buffer_insert_bytes(View *v, const char *buf, size_t len);
void buffer_delete_bytes(View *v, size_t len);
void buffer_erase_bytes(View *v, size_t len);
void buffer_replace_bytes(View *v, size_t del_count, const char *ins, size_t ins_count);

#endif
