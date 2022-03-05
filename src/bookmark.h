#ifndef BOOKMARK_H
#define BOOKMARK_H

#include <stdbool.h>
#include "util/macros.h"
#include "util/ptr-array.h"
#include "view.h"

typedef struct {
    // Needed after buffer is closed
    char *filename;

    // Needed if buffer doesn't have filename.
    // Pointer would have to be set to NULL after closing buffer.
    unsigned long buffer_id;

    // If pattern is set then line and column are 0 and vice versa
    char *pattern; // Regex from tag file
    unsigned long line, column;
} FileLocation;

FileLocation *get_current_file_location(const View *view) NONNULL_ARGS_AND_RETURN;
bool file_location_go(const FileLocation *loc) NONNULL_ARGS;
void file_location_free(FileLocation *loc) NONNULL_ARGS;

void bookmark_push(PointerArray *bookmarks, FileLocation *loc) NONNULL_ARGS;;
void bookmark_pop(PointerArray *bookmarks) NONNULL_ARGS;;

#endif
