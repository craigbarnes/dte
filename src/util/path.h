#ifndef UTIL_PATH_H
#define UTIL_PATH_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "macros.h"
#include "string-view.h"
#include "xmalloc.h"

NONNULL_ARGS
static inline bool path_is_absolute(const char *path)
{
    return path[0] == '/';
}

// filename must not contain trailing slashes (but it can be "/")
NONNULL_ARGS_AND_RETURN
static inline const char *path_basename(const char *filename)
{
    const char *slash = strrchr(filename, '/');
    return slash ? slash + 1 : filename;
}

NONNULL_ARGS
static inline StringView path_slice_dirname(const char *filename)
{
    const char *const slash = strrchr(filename, '/');
    if (slash == NULL) {
        return string_view(".", 1);
    }
    bool slash_is_root_dir = (slash == filename);
    size_t dirname_length = slash_is_root_dir ? 1 : slash - filename;
    return string_view(filename, dirname_length);
}

XSTRDUP
static inline char *path_dirname(const char *filename)
{
    const StringView dir = path_slice_dirname(filename);
    return xstrcut(dir.data, dir.length);
}

char *path_absolute(const char *filename) MALLOC NONNULL_ARGS;
char *relative_filename(const char *f, const char *cwd) XSTRDUP;

#endif
