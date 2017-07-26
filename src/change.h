#ifndef CHANGE_H
#define CHANGE_H

#include "libc.h"

enum change_merge {
	CHANGE_MERGE_NONE,
	CHANGE_MERGE_INSERT,
	CHANGE_MERGE_DELETE,
	CHANGE_MERGE_ERASE,
};

struct change;

void begin_change(enum change_merge m);
void end_change(void);
void begin_change_chain(void);
void end_change_chain(void);
bool undo(void);
bool redo(unsigned int change_id);
void free_changes(struct change *head);
void buffer_insert_bytes(const char *buf, long len);
void buffer_delete_bytes(long len);
void buffer_erase_bytes(long len);
void buffer_replace_bytes(long del_count, const char *inserted, long ins_count);

#endif
