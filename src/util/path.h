#ifndef UTIL_PATH_H
#define UTIL_PATH_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "debug.h"
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
    const char *slash = strrchr(filename, '/');
    if (!slash) {
        return strview(".");
    }
    bool slash_is_root_dir = (slash == filename);
    return string_view(filename, slash_is_root_dir ? 1 : slash - filename);
}

XSTRDUP
static inline char *path_dirname(const char *filename)
{
    const StringView dir = path_slice_dirname(filename);
    return xstrcut(dir.data, dir.length);
}

static inline char *path_join_sv(StringView s1, StringView s2, bool trailing_slash)
{
    const char slash[2] = "/";
    size_t n1 = s1.length;
    size_t n2 = s2.length;
    size_t sep = n1 && n2 && s1.data[n1 - 1] != '/'; // Separating slash length (1 or 0)
    size_t ts = trailing_slash && n2 && s2.data[n2 - 1] != '/'; // Trailing slash length (1 or 0)
    char *path = xmalloc(xadd3(n1, n2, sep + ts + 1));
    xmempcpy4(path, s1.data, n1, slash, sep, s2.data, n2, slash + !ts, 2 - !ts);
    return path;
}

static inline char *path_join(const char *s1, const char *s2)
{
    return path_join_sv(strview(s1), strview(s2), false);
}

// If path is the root directory, return false. Otherwise, mutate the
// path argument to become its parent directory and return true. Note
// that path *must* be canonical (i.e. as returned by path_absolute()).
static inline bool path_parent(StringView *path)
{
    BUG_ON(!strview_has_prefix(path, "/"));
    if (unlikely(path->length == 1)) {
        return false; // Root dir
    }

    // Remove up to 1 trailing slash
    if (unlikely(strview_remove_matching_suffix(path, "/"))) {
        BUG_ON(strview_has_suffix(path, "/"));
    }

    // Adjust the length, to exclude the last path component
    ssize_t slash_idx = strview_memrchr_idx(path, '/');
    BUG_ON(slash_idx < 0);
    path->length = MAX(slash_idx, 1); // Shortest valid path is "/"
    return true;
}

char *path_absolute(const char *path) MALLOC NONNULL_ARGS;
char *path_relative(const char *absolute, const char *cwd) XSTRDUP;
char *short_filename(const char *absolute, StringView home_dir) XSTRDUP;
char *short_filename_cwd(const char *absolute, const char *cwd, StringView home_dir) XSTRDUP;
const char *path_slice_relative(const char *abs, const char *cwd) NONNULL_ARGS RETURNS_NONNULL;

#endif
