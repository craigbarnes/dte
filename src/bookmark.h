#ifndef BOOKMARK_H
#define BOOKMARK_H

#include <stdbool.h>
#include "util/macros.h"
#include "util/ptr-array.h"
#include "view.h"
#include "window.h"

typedef struct {
    char *filename; // Needed after buffer is closed
    unsigned long buffer_id; // Needed if buffer doesn't have a filename
    char *pattern; // Regex from tag file (if set, line and column are 0)
    unsigned long line, column; // File position (if non-zero, pattern is NULL)
} FileLocation;

FileLocation *get_current_file_location(const View *view) NONNULL_ARGS_AND_RETURN;
bool file_location_go(Window *window, const FileLocation *loc) NONNULL_ARGS WARN_UNUSED_RESULT;
void file_location_free(FileLocation *loc) NONNULL_ARGS;

void bookmark_push(PointerArray *bookmarks, FileLocation *loc) NONNULL_ARGS;
void bookmark_pop(Window *window, PointerArray *bookmarks) NONNULL_ARGS;

#endif
