#include "filetype.h"
#include "common.h"
#include "regexp.h"
#include "path.h"
#include "ptr-array.h"

// Single filetype and extension/regexp pair.
// Filetypes are not grouped by name to make it possible to order them freely.
typedef struct {
    char *name;
    char *str;
    enum detect_type type;
} FileType;

static PointerArray filetypes = PTR_ARRAY_INIT;

static const char *const ignore[] = {
    "bak", "dpkg-dist", "dpkg-old", "new", "old", "orig", "pacnew",
    "pacorig", "pacsave", "rpmnew", "rpmsave",
};

void add_filetype(const char *name, const char *str, enum detect_type type)
{
    FileType *ft;
    regex_t re;

    switch (type) {
    case FT_CONTENT:
    case FT_FILENAME:
        if (!regexp_compile(&re, str, REG_NEWLINE | REG_NOSUB)) {
            return;
        }
        regfree(&re);
        break;
    default:
        break;
    }

    ft = xnew(FileType, 1);
    ft->name = xstrdup(name);
    ft->str = xstrdup(str);
    ft->type = type;
    ptr_array_add(&filetypes, ft);
}

// file.c.old~ -> c
// file..old   -> old
// file.old    -> old
static char *get_ext(const char *const filename)
{
    const char *ext = strrchr(filename, '.');

    if (!ext) {
        return NULL;
    }

    ext++;
    size_t ext_len = strlen(ext);
    if (ext_len && ext[ext_len - 1] == '~') {
        ext_len--;
    }
    if (!ext_len) {
        return NULL;
    }

    for (size_t i = 0; i < ARRAY_COUNT(ignore); i++) {
        if (!strncmp(ignore[i], ext, ext_len) && !ignore[i][ext_len]) {
            int idx = -2;
            while (ext + idx >= filename) {
                if (ext[idx] == '.') {
                    int len = -idx - 2;
                    if (len) {
                        ext -= len + 1;
                        ext_len = len;
                    }
                    break;
                }
                idx--;
            }
            break;
        }
    }
    return xstrslice(ext, 0, ext_len);
}

const char *find_ft (
    const char *filename,
    const char *interpreter,
    const char *first_line,
    size_t line_len
) {
    size_t filename_len = 0;
    const char *base = NULL;
    char *ext = NULL;
    if (filename) {
        base = path_basename(filename);
        ext = get_ext(filename);
        filename_len = strlen(filename);
    }
    for (size_t i = 0; i < filetypes.count; i++) {
        const FileType *ft = filetypes.ptrs[i];
        switch (ft->type) {
        case FT_EXTENSION:
            if (!ext || !streq(ext, ft->str)) {
                continue;
            }
            break;
        case FT_BASENAME:
            if (!base || !streq(base, ft->str)) {
                continue;
            }
            break;
        case FT_FILENAME:
            if (
                !filename
                || !regexp_match_nosub(ft->str, filename, filename_len)
            ) {
                continue;
            }
            break;
        case FT_CONTENT:
            if (
                !first_line
                || !regexp_match_nosub(ft->str, first_line, line_len)
            ) {
                continue;
            }
            break;
        case FT_INTERPRETER:
            if (!interpreter || !streq(interpreter, ft->str)) {
                continue;
            }
            break;
        }
        free(ext);
        return ft->name;
    }
    free(ext);
    return NULL;
}

bool is_ft(const char *name)
{
    for (size_t i = 0; i < filetypes.count; i++) {
        const FileType *ft = filetypes.ptrs[i];
        if (streq(ft->name, name)) {
            return true;
        }
    }
    return false;
}
