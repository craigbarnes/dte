#include <stdint.h>
#include <stdlib.h>
#include "filetype.h"
#include "command/serialize.h"
#include "regexp.h"
#include "util/ascii.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/hashset.h"
#include "util/log.h"
#include "util/path.h"
#include "util/str-util.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xmemmem.h"

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
// NOLINTBEGIN(bugprone-suspicious-include)
#include "filetype/names.c"
#include "filetype/basenames.c"
#include "filetype/directories.c"
#include "filetype/extensions.c"
#include "filetype/interpreters.c"
#include "filetype/ignored-exts.c"
#include "filetype/signatures.c"
// NOLINTEND(bugprone-suspicious-include)

UNITTEST {
    static_assert(NR_BUILTIN_FILETYPES < 256);
    CHECK_BSEARCH_ARRAY(basenames, name);
    CHECK_BSEARCH_ARRAY(extensions, ext);
    CHECK_BSEARCH_ARRAY(interpreters, key);
    CHECK_BSEARCH_ARRAY(emacs_modes, name);
    CHECK_BSEARCH_STR_ARRAY(ignored_extensions);
    CHECK_BSEARCH_STR_ARRAY(builtin_filetype_names);

    for (size_t i = 0; i < ARRAYLEN(builtin_filetype_names); i++) {
        const char *name = builtin_filetype_names[i];
        if (unlikely(!is_valid_filetype_name(name))) {
            BUG("invalid name at builtin_filetype_names[%zu]: \"%s\"", i, name);
        }
    }
}

typedef struct {
    unsigned int str_len;
    char str[] COUNTED_BY(str_len);
} FlexArrayStr;

// Filetypes dynamically added via the `ft` command.
// Not grouped by name to make it possible to order them freely.
typedef struct {
    union {
        FlexArrayStr *str;
        const InternedRegexp *regexp;
    } u;
    uint8_t type; // FileDetectionType
    char name[];
} UserFileTypeEntry;

static bool ft_uses_regex(FileDetectionType type)
{
    return type == FT_CONTENT || type == FT_FILENAME;
}

bool add_filetype (
    PointerArray *filetypes,
    const char *name,
    const char *str,
    FileDetectionType type,
    ErrorBuffer *ebuf
) {
    BUG_ON(!is_valid_filetype_name(name));
    const InternedRegexp *ir = NULL;
    if (ft_uses_regex(type)) {
        ir = regexp_intern(ebuf, str);
        if (unlikely(!ir)) {
            return false;
        }
    }

    size_t name_len = strlen(name);
    UserFileTypeEntry *ft = xmalloc(sizeof(*ft) + name_len + 1);
    ft->type = type;

    if (ir) {
        ft->u.regexp = ir;
    } else {
        size_t str_len = strlen(str);
        FlexArrayStr *s = xmalloc(sizeof(*s) + str_len + 1);
        s->str_len = str_len;
        ft->u.str = s;
        memcpy(s->str, str, str_len + 1);
    }

    memcpy(ft->name, name, name_len + 1);
    ptr_array_append(filetypes, ft);
    return true;
}

static StringView path_extension(StringView filename)
{
    StringView ext = filename;
    ssize_t pos = strview_memrchr_idx(&ext, '.');
    strview_remove_prefix(&ext, pos > 0 ? pos + 1 : ext.length);
    return ext;
}

static StringView get_filename_extension(StringView filename)
{
    StringView ext = path_extension(filename);
    if (is_ignored_extension(ext)) {
        filename.length -= ext.length + 1;
        ext = path_extension(filename);
    }
    strview_remove_matching_suffix(&ext, "~");
    return ext;
}

// Parse hashbang and return interpreter name, without version number.
// For example, if line is "#!/usr/bin/env python2", "python" is returned.
static StringView get_interpreter(StringView line)
{
    StringView sv = STRING_VIEW_INIT;
    if (!strview_remove_matching_prefix(&line, "#!")) {
        return sv;
    }

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
    const FlexArrayStr *s = ft->u.str;
    return sv.length > 0 && strview_equal_strn(&sv, s->str, s->str_len);
}

static bool ft_regex_match(const UserFileTypeEntry *ft, const StringView sv)
{
    const regex_t *re = &ft->u.regexp->re;
    return sv.length > 0 && regexp_exec(re, sv.data, sv.length, 0, NULL, 0);
}

static bool ft_match(const UserFileTypeEntry *ft, const StringView sv)
{
    FileDetectionType t = ft->type;
    return ft_uses_regex(t) ? ft_regex_match(ft, sv) : ft_str_match(ft, sv);
}

typedef FileTypeEnum (*FileTypeLookupFunc)(const StringView sv);

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
    static const FileTypeLookupFunc funcs[] = {
        [FT_INTERPRETER] = filetype_from_interpreter,
        [FT_BASENAME] = filetype_from_basename,
        [FT_CONTENT] = filetype_from_signature,
        [FT_EXTENSION] = filetype_from_extension,
        [FT_FILENAME] = filetype_from_dir_prefix,
    };

    const StringView params[] = {
        [FT_INTERPRETER] = interpreter,
        [FT_BASENAME] = base,
        [FT_CONTENT] = line,
        [FT_EXTENSION] = ext,
        [FT_FILENAME] = path,
    };

    // Search user `ft` entries
    for (size_t i = 0, n = filetypes->count; i < n; i++) {
        const UserFileTypeEntry *ft = filetypes->ptrs[i];
        if (ft_match(ft, params[ft->type])) {
            return ft->name;
        }
    }

    // Search built-in lookup tables
    static_assert(ARRAYLEN(funcs) == ARRAYLEN(params));
    for (FileDetectionType i = 0; i < ARRAYLEN(funcs); i++) {
        BUG_ON(!funcs[i]);
        FileTypeEnum ft = funcs[i](params[i]);
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
    // Insert all filetype names beginning with `prefix` into a HashSet
    // (to avoid duplicates)
    HashSet set;
    size_t prefix_len = strlen(prefix);
    size_t nr_builtin_ft = ARRAYLEN(builtin_filetype_names);
    hashset_init(&set, 20 + (prefix[0] == '\0' ? nr_builtin_ft : 0), false);

    for (size_t i = 0; i < nr_builtin_ft; i++) {
        const char *name = builtin_filetype_names[i];
        if (str_has_strn_prefix(name, prefix, prefix_len)) {
            hashset_insert(&set, name, strlen(name));
        }
    }

    for (size_t i = 0, n = filetypes->count; i < n; i++) {
        const UserFileTypeEntry *ft = filetypes->ptrs[i];
        const char *name = ft->name;
        if (str_has_strn_prefix(name, prefix, prefix_len)) {
            hashset_insert(&set, name, strlen(name));
        }
    }

    // Append the collected strings to the PointerArray
    for (HashSetIter iter = hashset_iter(&set); hashset_next(&iter); ) {
        ptr_array_append(a, xmemdup(iter.entry->str, iter.entry->str_len + 1));
    }

    hashset_free(&set);
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
    if (!ft_uses_regex(ft->type)) {
        free(ft->u.str);
    }
    free(ft);
}

void free_filetypes(PointerArray *filetypes)
{
    ptr_array_free_cb(filetypes, FREE_FUNC(free_filetype_entry));
}

bool is_valid_filetype_name_sv(StringView name)
{
    const char *data = name.data;
    const size_t len = name.length;
    if (unlikely(len == 0 || len > FILETYPE_NAME_MAX || data[0] == '-')) {
        return false;
    }

    const AsciiCharType mask = ASCII_SPACE | ASCII_CNTRL;
    for (size_t i = 0; i < len; i++) {
        unsigned char ch = data[i];
        if (unlikely(ascii_test(ch, mask) || ch == '/')) {
            return false;
        }
    }

    return true;
}

const char *filetype_str_from_extension(const char *path)
{
    StringView base = strview(path_basename(path));
    StringView ext = get_filename_extension(base);
    FileTypeEnum ft = filetype_from_extension(ext);
    return (ft == NONE) ? NULL : builtin_filetype_names[ft];
}
