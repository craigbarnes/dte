#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "path.h"
#include "str-util.h"

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

    // If the full path doesn't exist but the directory part does, return
    // the concatenation of the real directory and the non-existent filename
    abs = path_join(dir, base);
    free(dir);
    return abs;
}

static bool path_component(const char *path, size_t pos)
{
    return path[pos] == '\0' || pos == 0 || path[pos - 1] == '/';
}

// Like path_relative(), but always returning either a "borrowed" pointer
// into `abs` or the static string ".", thus avoiding the need to allocate.
// This most notably means that "../" path components are never used, even
// if doing so would produce a path shorter than `abs`.
const char *path_slice_relative(const char *abs, const char *cwd)
{
    BUG_ON(!path_is_absolute(cwd));
    BUG_ON(!path_is_absolute(abs));

    // Special case
    if (cwd[1] == '\0') {
        return abs[1] == '\0' ? abs : abs + 1;
    }

    size_t clen = str_common_prefix_length(cwd, abs);
    if (cwd[clen] == '\0') {
        switch (abs[clen]) {
        case '\0':
            // Identical strings; abs is current directory
            return ".";
        case '/':
            return abs + clen + 1;
        }
    }

    return abs;
}

char *path_relative(const char *abs, const char *cwd)
{
    BUG_ON(!path_is_absolute(cwd));
    BUG_ON(!path_is_absolute(abs));

    // Special case
    if (cwd[1] == '\0') {
        return xstrdup(abs[1] == '\0' ? abs : abs + 1);
    }

    size_t clen = str_common_prefix_length(cwd, abs);
    if (cwd[clen] == '\0') {
        switch (abs[clen]) {
        case '\0':
            // Identical strings; abs is current directory
            return xstrdup(".");
        case '/':
            // cwd    = /home/user
            // abs    = /home/user/project-a/file.c
            // common = /home/user
            return xstrdup(abs + clen + 1);
        }
    }

    // Common path components
    if (!path_component(cwd, clen) || !path_component(abs, clen)) {
        while (clen > 0 && abs[clen - 1] != '/') {
            clen--;
        }
    }

    // Number of "../" needed
    size_t dotdot = 1;
    const char *slash = strchr(cwd + clen + 1, '/');
    if (slash) {
        dotdot++;
        if (strchr(slash + 1, '/')) {
            // Just use absolute path if `dotdot` would be > 2
            return xstrdup(abs);
        }
    }

    const char *tail = abs + clen;
    return xmemjoin("../../", 3 * dotdot, tail, strlen(tail) + 1);
}

static bool path_has_dir_prefix(StringView path, StringView dir)
{
    return strview_has_sv_prefix(path, dir) && path.data[dir.length] == '/';
}

char *short_filename_cwd(const char *abs, const char *cwd, StringView home)
{
    StringView abs_sv = strview(abs);
    char *rel = path_relative(abs, cwd);
    size_t abs_len = abs_sv.length;
    size_t rel_len = strlen(rel);
    size_t suffix_len = (abs_len - home.length) + 1;

    if (path_has_dir_prefix(abs_sv, home) && suffix_len < rel_len) {
        // Prefer absolute in tilde notation (e.g. "~/abs/path"), if applicable
        // and shorter than relative
        rel[0] = '~';
        memcpy(rel + 1, abs + home.length, suffix_len);
    } else if (abs_len < rel_len) {
        // Prefer absolute, if shorter than relative
        memcpy(rel, abs, abs_len + 1);
    }

    return rel;
}

char *short_filename(const char *abs, StringView home)
{
    char buf[8192];
    const char *cwd = getcwd(buf, sizeof buf);
    return likely(cwd) ? short_filename_cwd(abs, cwd, home) : xstrdup(abs);
}
