#include <stdint.h>
#include <stdlib.h>
#include "filetype.h"
#include "command/serialize.h"
#include "regexp.h"
#include "util/array.h"
#include "util/ascii.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/path.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

static int ft_compare(const void *key, const void *elem)
{
    const StringView *sv = key;
    const char *ext = elem; // Cast to first member of struct
    int res = memcmp(sv->data, ext, sv->length);
    if (unlikely(res == 0 && ext[sv->length] != '\0')) {
        res = -1;
    }
    return res;
}

// Built-in filetypes
#include "filetype/names.c"
#include "filetype/basenames.c"
#include "filetype/directories.c"
#include "filetype/extensions.c"
#include "filetype/interpreters.c"
#include "filetype/ignored-exts.c"
#include "filetype/signatures.c"

UNITTEST {
    static_assert(NR_BUILTIN_FILETYPES < 256);
    CHECK_BSEARCH_ARRAY(basenames, name, strcmp);
    CHECK_BSEARCH_ARRAY(extensions, ext, strcmp);
    CHECK_BSEARCH_ARRAY(interpreters, key, strcmp);
    CHECK_BSEARCH_STR_ARRAY(ignored_extensions, strcmp);
    CHECK_BSEARCH_STR_ARRAY(builtin_filetype_names, strcmp);

    for (size_t i = 0; i < ARRAYLEN(builtin_filetype_names); i++) {
        const char *name = builtin_filetype_names[i];
        if (unlikely(!is_valid_filetype_name(name))) {
            BUG("invalid name at builtin_filetype_names[%zu]: \"%s\"", i, name);
        }
    }
}

typedef struct {
    unsigned int str_len;
    char str[];
} FlexArrayStr;

// Filetypes dynamically added via the `ft` command.
// Not grouped by name to make it possible to order them freely.
typedef struct {
    union {
        FlexArrayStr *str;
        CachedRegexp *regexp;
    } u;
    uint8_t type; // FileDetectionType
    char name[];
} UserFileTypeEntry;

static bool ft_uses_regex(FileDetectionType type)
{
    return type == FT_CONTENT || type == FT_FILENAME;
}

bool add_filetype(PointerArray *filetypes, const char *name, const char *str, FileDetectionType type)
{
    BUG_ON(!is_valid_filetype_name(name));
    regex_t re;
    bool use_re = ft_uses_regex(type);
    if (use_re) {
        int err = regcomp(&re, str, DEFAULT_REGEX_FLAGS | REG_NEWLINE | REG_NOSUB);
        if (unlikely(err)) {
            return regexp_error_msg(&re, str, err);
        }
    }

    size_t name_len = strlen(name);
    size_t str_len = strlen(str);
    UserFileTypeEntry *ft = xmalloc(sizeof(*ft) + name_len + 1);
    ft->type = type;

    char *str_dest;
    if (use_re) {
        CachedRegexp *r = xmalloc(sizeof(*r) + str_len + 1);
        r->re = re;
        ft->u.regexp = r;
        str_dest = r->str;
    } else {
        FlexArrayStr *s = xmalloc(sizeof(*s) + str_len + 1);
        s->str_len = str_len;
        ft->u.str = s;
        str_dest = s->str;
    }

    memcpy(ft->name, name, name_len + 1);
    memcpy(str_dest, str, str_len + 1);
    ptr_array_append(filetypes, ft);
    return true;
}

static StringView path_extension(StringView filename)
{
    StringView ext = STRING_VIEW_INIT;
    ext.data = strview_memrchr(&filename, '.');
    if (!ext.data || ext.data == filename.data) {
        return ext;
    }
    ext.data++;
    ext.length = filename.length - (ext.data - filename.data);
    return ext;
}

static StringView get_filename_extension(StringView filename)
{
    StringView ext = path_extension(filename);
    if (is_ignored_extension(ext)) {
        filename.length -= ext.length + 1;
        ext = path_extension(filename);
    }
    if (strview_has_suffix(&ext, "~")) {
        ext.length--;
    }
    return ext;
}

// Parse hashbang and return interpreter name, without version number.
// For example, if line is "#!/usr/bin/env python2", "python" is returned.
static StringView get_interpreter(StringView line)
{
    StringView sv = STRING_VIEW_INIT;
    if (!strview_has_prefix(&line, "#!")) {
        return sv;
    }

    strview_remove_prefix(&line, 2);
    strview_trim_left(&line);
    if (line.length < 2 || line.data[0] != '/') {
        return sv;
    }

    size_t pos = 0;
    sv = get_delim(line.data, &pos, line.length, ' ');
    if (pos < line.length && strview_equal_cstring(&sv, "/usr/bin/env")) {
        while (pos + 1 < line.length && line.data[pos] == ' ') {
            pos++;
        }
        sv = get_delim(line.data, &pos, line.length, ' ');
    }

    ssize_t last_slash_idx = strview_memrchr_idx(&sv, '/');
    if (last_slash_idx >= 0) {
        strview_remove_prefix(&sv, last_slash_idx + 1);
    }

    while (sv.length && ascii_is_digit_or_dot(sv.data[sv.length - 1])) {
        sv.length--;
    }

    return sv;
}

static bool ft_str_match(const UserFileTypeEntry *ft, const StringView sv)
{
    const char *str = ft->u.str->str;
    const size_t len = ft->u.str->str_len;
    return sv.length > 0 && strview_equal_strn(&sv, str, len);
}

static bool ft_regex_match(const UserFileTypeEntry *ft, const StringView sv)
{
    const regex_t *re = &ft->u.regexp->re;
    regmatch_t m;
    return sv.length > 0 && regexp_exec(re, sv.data, sv.length, 0, &m, 0);
}

static bool ft_match(const UserFileTypeEntry *ft, const StringView sv)
{
    if (ft_uses_regex(ft->type)) {
        return ft_regex_match(ft, sv);
    }
    return ft_str_match(ft, sv);
}

const char *find_ft(const PointerArray *filetypes, const char *filename, StringView line)
{
    const char *b = filename ? path_basename(filename) : NULL;
    const StringView base = strview_from_cstring(b);
    const StringView ext = get_filename_extension(base);
    const StringView path = strview_from_cstring(filename);
    const StringView interpreter = get_interpreter(line);
    BUG_ON(path.length == 0 && (base.length != 0 || ext.length != 0));
    BUG_ON(line.length == 0 && interpreter.length != 0);

    // The order of elements in this array determines the order of
    // precedence for the lookup() functions (but note that changing
    // the initializer below makes no difference to the array order)
    const struct {
        StringView sv;
        FileTypeEnum (*lookup)(const StringView sv);
    } table[] = {
        [FT_INTERPRETER] = {interpreter, filetype_from_interpreter},
        [FT_BASENAME] = {base, filetype_from_basename},
        [FT_CONTENT] = {line, filetype_from_signature},
        [FT_EXTENSION] = {ext, filetype_from_extension},
        [FT_FILENAME] = {path, filetype_from_dir_prefix},
    };

    // Search user `ft` entries
    for (size_t i = 0, n = filetypes->count; i < n; i++) {
        const UserFileTypeEntry *ft = filetypes->ptrs[i];
        if (ft_match(ft, table[ft->type].sv)) {
            return ft->name;
        }
    }

    // Search built-in lookup tables
    for (FileDetectionType i = 0; i < ARRAYLEN(table); i++) {
        BUG_ON(!table[i].lookup);
        FileTypeEnum ft = table[i].lookup(table[i].sv);
        if (ft != NONE) {
            return builtin_filetype_names[ft];
        }
    }

    // Use "ini" filetype if first line looks like an ini [section]
    strview_trim_right(&line);
    if (line.length >= 4) {
        const char *s = line.data;
        const size_t n = line.length;
        if (s[0] == '[' && s[n - 1] == ']' && is_word_byte(s[1])) {
            if (!strview_contains_char_type(&line, ASCII_CNTRL)) {
                return builtin_filetype_names[INI];
            }
        }
    }

    if (strview_equal_cstring(&ext, "conf")) {
        if (strview_has_prefix(&path, "/etc/systemd/")) {
            return builtin_filetype_names[INI];
        }
        BUG_ON(!filename);
        const StringView dir = path_slice_dirname(filename);
        if (
            strview_has_prefix(&path, "/etc/")
            || strview_has_prefix(&path, "/usr/share/")
            || strview_has_prefix(&path, "/usr/local/share/")
            || strview_has_suffix(&dir, "/tmpfiles.d")
        ) {
            return builtin_filetype_names[CONFIG];
        }
    }

    return NULL;
}

bool is_ft(const PointerArray *filetypes, const char *name)
{
    if (BSEARCH(name, builtin_filetype_names, vstrcmp)) {
        return true;
    }

    for (size_t i = 0, n = filetypes->count; i < n; i++) {
        const UserFileTypeEntry *ft = filetypes->ptrs[i];
        if (streq(ft->name, name)) {
            return true;
        }
    }

    return false;
}

void collect_ft(const PointerArray *filetypes, PointerArray *a, const char *prefix)
{
    COLLECT_STRINGS(builtin_filetype_names, a, prefix);
    for (size_t i = 0, n = filetypes->count; i < n; i++) {
        const UserFileTypeEntry *ft = filetypes->ptrs[i];
        const char *name = ft->name;
        if (str_has_prefix(name, prefix)) {
            ptr_array_append(a, xstrdup(name));
        }
    }
}

static const char *ft_get_str(const UserFileTypeEntry *ft)
{
    return ft_uses_regex(ft->type) ? ft->u.regexp->str : ft->u.str->str;
}

String dump_filetypes(const PointerArray *filetypes)
{
    static const char flags[][4] = {
        [FT_EXTENSION] = "",
        [FT_FILENAME] = "-f ",
        [FT_CONTENT] = "-c ",
        [FT_INTERPRETER] = "-i ",
        [FT_BASENAME] = "-b ",
    };

    String s = string_new(4096);
    for (size_t i = 0, n = filetypes->count; i < n; i++) {
        const UserFileTypeEntry *ft = filetypes->ptrs[i];
        BUG_ON(ft->type >= ARRAYLEN(flags));
        BUG_ON(ft->name[0] == '-');
        string_append_literal(&s, "ft ");
        string_append_cstring(&s, flags[ft->type]);
        string_append_escaped_arg(&s, ft->name, true);
        string_append_byte(&s, ' ');
        string_append_escaped_arg(&s, ft_get_str(ft), true);
        string_append_byte(&s, '\n');
    }
    return s;
}

static void free_filetype_entry(UserFileTypeEntry *ft)
{
    if (ft_uses_regex(ft->type)) {
        free_cached_regexp(ft->u.regexp);
    } else {
        free(ft->u.str);
    }
    free(ft);
}

void free_filetypes(PointerArray *filetypes)
{
    ptr_array_free_cb(filetypes, FREE_FUNC(free_filetype_entry));
}
