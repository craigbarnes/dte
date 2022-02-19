#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "path.h"
#include "util/str-util.h"

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

char *relative_filename(const char *abs, const char *cwd)
{
    BUG_ON(!path_is_absolute(cwd));
    BUG_ON(!path_is_absolute(abs));

    // Special case
    if (cwd[1] == '\0') {
        return xstrdup(abs[1] == '\0' ? abs : abs + 1);
    }

    // Length of common path
    size_t clen = 0;
    while (cwd[clen] && cwd[clen] == abs[clen]) {
        clen++;
    }

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

    size_t hlen = 3 * dotdot;
    size_t tlen = strlen(abs + clen) + 1;
    char *filename = xmalloc(hlen + tlen);
    memcpy(filename, "../../", hlen);
    memcpy(filename + hlen, abs + clen, tlen);
    return filename;
}

char *short_filename_cwd(const char *abs, const char *cwd, const StringView *home)
{
    char *rel = relative_filename(abs, cwd);
    size_t abs_len = strlen(abs);
    size_t rel_len = strlen(rel);
    if (abs_len < rel_len) {
        // Prefer absolute if relative isn't shorter
        free(rel);
        rel = xstrdup(abs);
        rel_len = abs_len;
    }

    size_t home_len = home->length;
    if (strn_has_strview_prefix(abs, abs_len, home) && abs[home_len] == '/') {
        size_t suffix_len = (abs_len - home_len) + 1;
        if (suffix_len < rel_len) {
            // Prefer absolute path in tilde notation (e.g. "~/abs/path")
            // if shorter than relative
            free(rel);
            char *tilde = xmalloc(suffix_len + 1);
            tilde[0] = '~';
            memcpy(tilde + 1, abs + home_len, suffix_len);
            return tilde;
        }
    }

    return rel;
}

char *short_filename(const char *abs, const StringView *home)
{
    char buf[8192];
    const char *cwd = getcwd(buf, sizeof buf);
    return likely(cwd) ? short_filename_cwd(abs, cwd, home) : xstrdup(abs);
}
