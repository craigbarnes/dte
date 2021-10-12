#ifndef FILE_HISTORY_H
#define FILE_HISTORY_H

#include <stdbool.h>
#include "util/macros.h"

void file_history_add(unsigned long row, unsigned long col, const char *filename);
void file_history_load(const char *filename);
void file_history_save(const char *filename);
bool file_history_find(const char *filename, unsigned long *row, unsigned long *col) WARN_UNUSED_RESULT;

#endif
