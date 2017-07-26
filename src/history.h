#ifndef HISTORY_H
#define HISTORY_H

#include "ptr-array.h"
#include "libc.h"

#define search_history_size 100
#define command_history_size 500

extern struct ptr_array search_history;
extern struct ptr_array command_history;

void history_add(struct ptr_array *history, const char *text, int max_entries);
bool history_search_forward(struct ptr_array *history, int *pos, const char *text);
bool history_search_backward(struct ptr_array *history, int *pos, const char *text);
void history_load(struct ptr_array *history, const char *filename, int max_entries);
void history_save(struct ptr_array *history, const char *filename);

#endif
