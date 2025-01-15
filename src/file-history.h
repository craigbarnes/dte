#ifndef FILE_HISTORY_H
#define FILE_HISTORY_H

#include <stdbool.h>
#include "error.h"
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

void file_history_append(FileHistory *hist, unsigned long row, unsigned long col, const char *filename) NONNULL_ARGS;
void file_history_load(FileHistory *hist, ErrorBuffer *ebuf, char *filename, size_t size_limit) NONNULL_ARGS;
void file_history_save(const FileHistory *hist, ErrorBuffer *ebuf) NONNULL_ARGS;
bool file_history_find(const FileHistory *hist, const char *filename, unsigned long *row, unsigned long *col) NONNULL_ARGS WARN_UNUSED_RESULT;
void file_history_free(FileHistory *history) NONNULL_ARGS;
String file_history_dump(const FileHistory *history) NONNULL_ARGS;
String file_history_dump_relative(const FileHistory *history) NONNULL_ARGS;

#endif
