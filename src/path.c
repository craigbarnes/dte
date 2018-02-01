#include "editor.h"
#include "path.h"
#include "common.h"
#include "cconv.h"

static bool make_absolute(char *dst, size_t size, const char *src)
{
    const size_t len = strlen(src);
    size_t pos = 0;

    if (src[0] != '/') {
        if (!getcwd(dst, size)) {
            return false;
        }
        pos = strlen(dst);
        dst[pos++] = '/';
    }
    if (pos + len + 1 > size) {
        errno = ENAMETOOLONG;
        return false;
    }
    memcpy(dst + pos, src, len + 1);
    return true;
}

static size_t remove_double_slashes(char *str)
{
    size_t d = 0;
    for (size_t s = 0; str[s]; s++) {
        if (str[s] != '/' || str[s + 1] != '/') {
            str[d++] = str[s];
        }
    }
    str[d] = 0;
    return d;
}

/*
 * Canonicalizes path name
 *
 *   - Replaces double-slashes with one slash
 *   - Removes any "." or ".." path components
 *   - Makes path absolute
 *   - Expands symbolic links
 *   - Checks that all but the last expanded path component are directories
 *   - Last path component is allowed to not exist
 */
char *path_absolute(const char *filename)
{
    unsigned int depth = 0;
    char buf[8192];

    if (!make_absolute(buf, sizeof(buf), filename)) {
        return NULL;
    }

    remove_double_slashes(buf);

    // For each component:
    //   * Remove "."
    //   * Remove ".." and previous component
    //   * If symlink then replace with link destination and start over

    char *sp = buf + 1;
    while (*sp) {
        char *ep = strchr(sp, '/');
        bool last = !ep;

        if (ep) {
            *ep = 0;
        }
        if (streq(sp, ".")) {
            if (last) {
                *sp = 0;
                break;
            }
            memmove(sp, ep + 1, strlen(ep + 1) + 1);
            continue;
        }
        if (streq(sp, "..")) {
            if (sp != buf + 1) {
                // Not first component, remove previous component
                sp--;
                while (sp[-1] != '/') {
                    sp--;
                }
            }

            if (last) {
                *sp = 0;
                break;
            }
            memmove(sp, ep + 1, strlen(ep + 1) + 1);
            continue;
        }

        struct stat st;
        int rc = lstat(buf, &st);
        if (rc) {
            if (last && errno == ENOENT) {
                break;
            }
            return NULL;
        }

        if (S_ISLNK(st.st_mode)) {
            char target[8192];
            char tmp[8192];
            size_t total_len = 0;
            size_t buf_len = sp - 1 - buf;
            size_t rest_len = 0;
            size_t pos = 0;
            const char *rest = NULL;

            if (!last) {
                rest = ep + 1;
                rest_len = strlen(rest);
            }
            if (++depth > 8) {
                errno = ELOOP;
                return NULL;
            }
            ssize_t target_len = readlink(buf, target, sizeof(target));
            if (target_len < 0) {
                return NULL;
            }
            if (target_len == sizeof(target)) {
                errno = ENAMETOOLONG;
                return NULL;
            }
            target[target_len] = 0;

            // Calculate length
            if (target[0] != '/') {
                total_len = buf_len + 1;
            }
            total_len += target_len;
            if (rest) {
                total_len += 1 + rest_len;
            }
            if (total_len + 1 > sizeof(tmp)) {
                errno = ENAMETOOLONG;
                return NULL;
            }

            // Build new path
            if (target[0] != '/') {
                memcpy(tmp, buf, buf_len);
                pos += buf_len;
                tmp[pos++] = '/';
            }
            memcpy(tmp + pos, target, target_len);
            pos += target_len;
            if (rest) {
                tmp[pos++] = '/';
                memcpy(tmp + pos, rest, rest_len);
                pos += rest_len;
            }
            tmp[pos] = 0;
            pos = remove_double_slashes(tmp);

            // Restart
            memcpy(buf, tmp, pos + 1);
            sp = buf + 1;
            continue;
        }

        if (last) {
            break;
        }

        if (!S_ISDIR(st.st_mode)) {
            errno = ENOTDIR;
            return NULL;
        }

        *ep = '/';
        sp = ep + 1;
    }
    return xstrdup(buf);
}

static bool path_component(const char *path, size_t pos)
{
    return path[pos] == 0 || pos == 0 || path[pos - 1] == '/';
}

char *relative_filename(const char *f, const char *cwd)
{
    // Annoying special case
    if (cwd[1] == 0) {
        if (f[1] == 0) {
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

    char *filename = xnew(char, len + 1);
    for (size_t i = 0; i < dotdot; i++) {
        memcpy(filename + i * 3, "../", 3);
    }
    memcpy(filename + dotdot * 3, f + clen, tlen + 1);
    return filename;
}

char *short_filename_cwd(const char *absolute, const char *cwd)
{
    char *f = relative_filename(absolute, cwd);
    size_t home_len = strlen(editor.home_dir);
    size_t abs_len = strlen(absolute);
    size_t f_len = strlen(f);

    if (f_len >= abs_len) {
        // Prefer absolute if relative isn't shorter
        free(f);
        f = xstrdup(absolute);
        f_len = abs_len;
    }

    if (
        abs_len > home_len
        && !memcmp(absolute, editor.home_dir, home_len)
        && absolute[home_len] == '/'
    ) {
        size_t len = abs_len - home_len + 1;
        if (len < f_len) {
            char *filename = xnew(char, len + 1);
            filename[0] = '~';
            memcpy(filename + 1, absolute + home_len, len);
            free(f);
            return filename;
        }
    }
    return f;
}

char *short_filename(const char *absolute)
{
    char cwd[8192];

    if (getcwd(cwd, sizeof(cwd))) {
        return short_filename_cwd(absolute, cwd);
    }
    return xstrdup(absolute);
}

char *path_dirname(const char *filename)
{
    char *slash = strrchr(filename, '/');
    if (slash == NULL) {
        return xstrdup(".");
    }
    if (slash == filename) {
        return xstrdup("/");
    }
    return xstrcut(filename, slash - filename);
}

// filename must not contain trailing slashes (but it can be "/")
const char *path_basename(const char *filename)
{
    const char *slash = strrchr(filename, '/');
    if (slash == NULL) {
        return filename;
    }
    return slash + 1;
}
