#ifndef CHANGE_H
#define CHANGE_H

#include <stdbool.h>
#include <stddef.h>

enum change_merge {
    CHANGE_MERGE_NONE,
    CHANGE_MERGE_INSERT,
    CHANGE_MERGE_DELETE,
    CHANGE_MERGE_ERASE,
};

typedef struct Change Change;

struct Change {
    Change *next;
    Change **prev;
    unsigned int nr_prev;
    bool move_after; // Move after inserted text when undoing delete?
    long offset;
    long del_count;
    long ins_count;
    char *buf; // Deleted bytes (inserted bytes need not be saved)
};

void begin_change(enum change_merge m);
void end_change(void);
void begin_change_chain(void);
void end_change_chain(void);
bool undo(void);
bool redo(unsigned int change_id);
void free_changes(Change *head);
void buffer_insert_bytes(const char *buf, size_t len);
void buffer_delete_bytes(size_t len);
void buffer_erase_bytes(size_t len);

void buffer_replace_bytes (
    size_t del_count,
    const char *const inserted,
    size_t ins_count
);

#endif
