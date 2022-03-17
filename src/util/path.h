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

XSTRDUP
static inline char *path_join_sv(const StringView *s1, const StringView *s2)
{
    size_t n1 = s1->length;
    size_t n2 = s2->length;
    char *path = xmalloc(n1 + n2 + 2);
    memcpy(path, s1->data, n1);
    char *ptr = path + n1;
    if (n1 && n2 && s1->data[n1 - 1] != '/') {
        *ptr++ = '/';
    }
    memcpy(ptr, s2->data, n2);
    ptr[n2] = '\0';
    return path;
}

XSTRDUP
static inline char *path_join(const char *s1, const char *s2)
{
    StringView sv1 = strview_from_cstring(s1);
    StringView sv2 = strview_from_cstring(s2);
    return path_join_sv(&sv1, &sv2);
}

// If path is the root directory, return false. Otherwise, mutate
// the path argument to become its parent directory and return true.
// Note: path *must* be canonical (i.e. as returned by path_absolute()).
static inline bool path_parent(StringView *path)
{
    BUG_ON(!strview_has_prefix(path, "/"));
    if (unlikely(path->length == 1)) {
        return false; // Root dir
    }

    // Remove up to 1 trailing slash
    if (unlikely(strview_has_suffix(path, "/"))) {
        path->length--;
        BUG_ON(strview_has_suffix(path, "/"));
    }

    ssize_t slash_idx = strview_memrchr_idx(path, '/');
    BUG_ON(slash_idx < 0);
    path->length = slash_idx ? slash_idx : 1;
    return true;
}

char *path_absolute(const char *filename) MALLOC NONNULL_ARGS;
char *path_relative(const char *f, const char *cwd) XSTRDUP;
char *short_filename(const char *absolute, const StringView *home_dir) XSTRDUP;
char *short_filename_cwd(const char *absolute, const char *cwd, const StringView *home_dir) XSTRDUP;

#endif
