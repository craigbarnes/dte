#ifndef FILE_HISTORY_H
#define FILE_HISTORY_H

#include <stdbool.h>
#include "util/hashmap.h"
#include "util/macros.h"
#include "util/string.h"

typedef struct FileHistoryEntry {
    struct FileHistoryEntry *next;
    struct FileHistoryEntry *prev;
    char *filename;
    unsigned long row;
    unsigned long col;
} FileHistoryEntry;

typedef struct {
    char *filename;
    HashMap entries;
    FileHistoryEntry *first;
    FileHistoryEntry *last;
} FileHistory;

void file_history_append(FileHistory *hist, unsigned long row, unsigned long col, const char *filename);
void file_history_load(FileHistory *hist, char *filename, size_t size_limit);
void file_history_save(const FileHistory *hist);
bool file_history_find(const FileHistory *hist, const char *filename, unsigned long *row, unsigned long *col) WARN_UNUSED_RESULT;
void file_history_free(FileHistory *history);
String file_history_dump(const FileHistory *history);
String file_history_dump_relative(const FileHistory *history);

#endif
