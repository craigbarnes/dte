#include <stdint.h>
#include "filetype.h"
#include "common.h"
#include "debug.h"
#include "util/macros.h"
#include "util/path.h"
#include "util/ptr-array.h"
#include "util/regexp.h"
#include "util/string-view.h"
#include "util/xmalloc.h"

static int ft_compare(const void *key, const void *elem)
{
    const StringView *sv = key;
    const char *ext = elem; // Cast to first member of struct
    int res = memcmp(sv->data, ext, sv->length);
    if (res == 0 && ext[sv->length] != '\0') {
        return -1;
    }
    return res;
}

// Built-in filetypes
#include "filetype/names.c"
#include "filetype/basenames.c"
#include "filetype/extensions.c"
#include "filetype/interpreters.c"
#include "filetype/ignored-exts.c"
#include "filetype/signatures.c"

// Filetypes dynamically added via the `ft` command.
// Not grouped by name to make it possible to order them freely.
typedef struct {
    char *name;
    char *str;
    enum detect_type type;
} UserFileTypeEntry;

static PointerArray filetypes = PTR_ARRAY_INIT;

void add_filetype(const char *name, const char *str, enum detect_type type)
{
    UserFileTypeEntry *ft;
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

    ft = xnew(UserFileTypeEntry, 1);
    ft->name = xstrdup(name);
    ft->str = xstrdup(str);
    ft->type = type;
    ptr_array_add(&filetypes, ft);
}

static inline StringView get_ext(const StringView filename)
{
    StringView ext = STRING_VIEW_INIT;
    ext.data = memrchr_(filename.data, '.', filename.length);
    if (ext.data == NULL) {
        return ext;
    }

    ext.data++;
    ext.length = filename.length - (ext.data - filename.data);

    if (ext.length && ext.data[ext.length - 1] == '~') {
        ext.length--;
    }
    if (ext.length == 0) {
        return ext;
    }

    if (is_ignored_extension(ext.data, ext.length)) {
        int idx = -2;
        while (ext.data + idx >= filename.data) {
            if (ext.data[idx] == '.') {
                int len = -idx - 2;
                if (len) {
                    ext.data -= len + 1;
                    ext.length = len;
                }
                break;
            }
            idx--;
        }
    }

    return ext;
}

// Parse hashbang and return interpreter name, without version number.
// For example, if line is "#!/usr/bin/env python2", "python" is returned.
static StringView get_interpreter(const StringView line)
{
    static const char pat[] = "^#! */.*(/env +|/)([a-zA-Z0-9_-]+)[0-9.]*( |$)";
    static regex_t re;
    static bool compiled;
    if (!compiled) {
        compiled = regexp_compile(&re, pat, REG_NEWLINE);
        BUG_ON(!compiled);
        BUG_ON(re.re_nsub < 2);
    }

    StringView sv = STRING_VIEW_INIT;
    regmatch_t m[3];
    if (!regexp_exec(&re, line.data, line.length, ARRAY_COUNT(m), m, 0)) {
        return sv;
    }

    regoff_t start = m[2].rm_so;
    regoff_t end = m[2].rm_eo;
    BUG_ON(start < 0 || end < 0);
    sv = string_view(line.data + start, end - start);
    return sv;
}

HOT const char *find_ft(const char *filename, StringView line)
{
    StringView path = STRING_VIEW_INIT;
    StringView ext = STRING_VIEW_INIT;
    StringView base = STRING_VIEW_INIT;
    if (filename) {
        const char *b = path_basename(filename);
        base = string_view_from_cstring(b);
        ext = get_ext(base);
        path = string_view_from_cstring(filename);
    }

    StringView interpreter = STRING_VIEW_INIT;
    if (line.length >= 4 && line.data[0] == '#' && line.data[1] == '!') {
        interpreter = get_interpreter(line);
    }

    // Search user `ft` entries
    for (size_t i = 0; i < filetypes.count; i++) {
        const UserFileTypeEntry *ft = filetypes.ptrs[i];
        switch (ft->type) {
        case FT_EXTENSION:
            if (!ext.length || !string_view_equal_cstr(&ext, ft->str)) {
                continue;
            }
            break;
        case FT_BASENAME:
            if (!base.length || !string_view_equal_cstr(&base, ft->str)) {
                continue;
            }
            break;
        case FT_FILENAME:
            if (
                !path.length
                || !regexp_match_nosub(ft->str, path.data, path.length)
            ) {
                continue;
            }
            break;
        case FT_CONTENT:
            if (
                !line.length
                || !regexp_match_nosub(ft->str, line.data, line.length)
            ) {
                continue;
            }
            break;
        case FT_INTERPRETER:
            if (
                !interpreter.length
                || !string_view_equal_cstr(&interpreter, ft->str)
            ) {
                continue;
            }
            break;
        }
        return ft->name;
    }

    // Search built-in lookup tables
    if (interpreter.length) {
        FileTypeEnum ft = filetype_from_interpreter(interpreter.data, interpreter.length);
        if (ft) {
            return builtin_filetype_names[ft];
        }
    }

    if (base.length) {
        FileTypeEnum ft = filetype_from_basename(base.data, base.length);
        if (ft) {
            return builtin_filetype_names[ft];
        }
    }

    if (line.length) {
        FileTypeEnum ft = filetype_from_signature(line.data, line.length);
        if (ft) {
            return builtin_filetype_names[ft];
        }
    }

    if (ext.length) {
        FileTypeEnum ft = filetype_from_extension(ext.data, ext.length);
        if (ft) {
            return builtin_filetype_names[ft];
        }
    }

    if (string_view_has_literal_prefix(&path, "/etc/default/")) {
        return builtin_filetype_names[SHELL];
    } else if (string_view_has_literal_prefix(&path, "/etc/nginx/")) {
        return builtin_filetype_names[NGINX];
    }

    const StringView conf = STRING_VIEW("conf");
    if (string_view_equal(&ext, &conf)) {
        if (string_view_has_literal_prefix(&path, "/etc/systemd/")) {
            return builtin_filetype_names[INI];
        } else if (string_view_has_literal_prefix(&path, "/etc/")) {
            return builtin_filetype_names[CONFIG];
        }
    }

    return NULL;
}

bool is_ft(const char *name)
{
    if (name[0] == '\0') {
        return false;
    }
    for (size_t i = 0; i < filetypes.count; i++) {
        const UserFileTypeEntry *ft = filetypes.ptrs[i];
        if (streq(ft->name, name)) {
            return true;
        }
    }
    for (size_t i = 1; i < ARRAY_COUNT(builtin_filetype_names); i++) {
        if (streq(builtin_filetype_names[i], name)) {
            return true;
        }
    }
    return false;
}
