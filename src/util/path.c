#define _XOPEN_SOURCE 700
#include <errno.h>
#include <stdlib.h>
#include "path.h"

char *path_absolute(const char *path)
{
    if (unlikely(path[0] == '\0')) {
        errno = EINVAL;
        return NULL;
    }

    char *abs = realpath(path, NULL);
    if (abs || errno != ENOENT) {
        return abs;
    }

    const char *base = path_basename(path);
    char *dir_relative = path_dirname(path);
    char *dir = realpath(dir_relative, NULL);
    free(dir_relative);
    if (!dir) {
        errno = ENOENT;
        return NULL;
    }

    // If the full path doesn't exist but the directory part does,
    // return the concatenation of the real directory and the
    // non-existent filename
    abs = path_join(dir, base);
    free(dir);
    return abs;
}

static bool path_component(const char *path, size_t pos)
{
    return path[pos] == '\0' || pos == 0 || path[pos - 1] == '/';
}

char *relative_filename(const char *f, const char *cwd)
{
    // Annoying special case
    if (cwd[1] == '\0') {
        if (f[1] == '\0') {
            return xstrdup(f);
        }
        return xstrdup(f + 1);
    }

    // Length of common path
    size_t clen = 0;
    while (cwd[clen] && cwd[clen] == f[clen]) {
        clen++;
    }

    if (!cwd[clen] && f[clen] == '/') {
        // cwd    = /home/user
        // abs    = /home/user/project-a/file.c
        // common = /home/user
        return xstrdup(f + clen + 1);
    }

    // Common path components
    if (!path_component(cwd, clen) || !path_component(f, clen)) {
        while (clen > 0 && f[clen - 1] != '/') {
            clen--;
        }
    }

    // Number of "../" needed
    size_t dotdot = 1;
    for (size_t i = clen + 1; cwd[i]; i++) {
        if (cwd[i] == '/') {
            dotdot++;
        }
    }
    if (dotdot > 2) {
        return xstrdup(f);
    }

    size_t tlen = strlen(f + clen);
    size_t len = dotdot * 3 + tlen;

    char *filename = xmalloc(len + 1);
    for (size_t i = 0; i < dotdot; i++) {
        memcpy(filename + i * 3, "../", 3);
    }
    memcpy(filename + dotdot * 3, f + clen, tlen + 1);
    return filename;
}
