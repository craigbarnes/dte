#ifndef BOOKMARK_H
#define BOOKMARK_H

#include <stdbool.h>
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/xmalloc.h"
#include "view.h"
#include "window.h"

typedef struct {
    char *filename; // Needed after buffer is closed
    unsigned long buffer_id; // Needed if buffer doesn't have a filename
    char *pattern; // Regex from tag file (if set, line and column are 0)
    unsigned long line, column; // File position (if non-zero, pattern is NULL)
} FileLocation;

static inline FileLocation *new_file_location (
    char *filename, // NOLINT(readability-non-const-parameter): false positive
    unsigned long buffer_id,
    unsigned long line,
    unsigned long column
) {
    FileLocation *loc = xmalloc(sizeof(*loc));
    *loc = (FileLocation) {
        .filename = filename,
        .buffer_id = buffer_id,
        .line = line,
        .column = column
    };
    return loc;
}

FileLocation *get_current_file_location(const View *view) NONNULL_ARGS_AND_RETURN;
bool file_location_go(Window *window, const FileLocation *loc) NONNULL_ARGS WARN_UNUSED_RESULT;
void file_location_free(FileLocation *loc) NONNULL_ARGS;

void bookmark_push(PointerArray *bookmarks, FileLocation *loc) NONNULL_ARGS;
void bookmark_pop(PointerArray *bookmarks, Window *window) NONNULL_ARGS;

#endif
