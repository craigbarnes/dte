#include <string.h>
#include "filetype.h"
#include "command/serialize.h"
#include "completion.h"
#include "error.h"
#include "regexp.h"
#include "util/ascii.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/macros.h"
#include "util/path.h"
#include "util/ptr-array.h"
#include "util/str-util.h"
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

UNITTEST {
    CHECK_BSEARCH_ARRAY(basenames, name, strcmp);
    CHECK_BSEARCH_ARRAY(extensions, ext, strcmp);
    CHECK_BSEARCH_ARRAY(interpreters, key, strcmp);
    CHECK_BSEARCH_STR_ARRAY(builtin_filetype_names, strcmp);
    CHECK_BSEARCH_STR_ARRAY(ignored_extensions, strcmp);
}

typedef struct {
    regex_t re;
    char str[];
} CachedRegexp;

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

static PointerArray filetypes = PTR_ARRAY_INIT;

static bool ft_uses_regex(FileDetectionType type)
{
    return type == FT_CONTENT || type == FT_FILENAME;
}

void add_filetype(const char *name, const char *str, FileDetectionType type)
{
    regex_t re;
    bool use_re = ft_uses_regex(type);
    if (use_re) {
        int err = regcomp(&re, str, REG_EXTENDED | REG_NEWLINE | REG_NOSUB);
        if (unlikely(err)) {
            regexp_error_msg(&re, str, err);
            return;
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
    ptr_array_append(&filetypes, ft);
}

static StringView get_ext(const StringView filename)
{
    StringView ext = STRING_VIEW_INIT;
    if (filename.length < 3) {
        return ext;
    }

    ext.data = strview_memrchr(&filename, '.');
    if (!ext.data) {
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

    if (is_ignored_extension(ext)) {
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

    const unsigned char *last_slash = strview_memrchr(&sv, '/');
    if (last_slash) {
        strview_remove_prefix(&sv, (last_slash - sv.data) + 1);
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

HOT const char *find_ft(const char *filename, StringView line)
{
    const char *b = filename ? path_basename(filename) : NULL;
    const StringView base = strview_from_cstring(b);
    const StringView ext = get_ext(base);
    const StringView path = strview_from_cstring(filename);
    const StringView interpreter = get_interpreter(line);

    // Search user `ft` entries
    for (size_t i = 0, n = filetypes.count; i < n; i++) {
        const UserFileTypeEntry *ft = filetypes.ptrs[i];
        switch (ft->type) {
        case FT_EXTENSION:
            if (!ft_str_match(ft, ext)) {
                continue;
            }
            break;
        case FT_BASENAME:
            if (!ft_str_match(ft, base)) {
                continue;
            }
            break;
        case FT_FILENAME:
            if (!ft_regex_match(ft, path)) {
                continue;
            }
            break;
        case FT_CONTENT:
            if (!ft_regex_match(ft, line)) {
                continue;
            }
            break;
        case FT_INTERPRETER:
            if (!ft_str_match(ft, interpreter)) {
                continue;
            }
            break;
        default:
            BUG("unhandled detection type");
        }
        return ft->name;
    }

    // Search built-in lookup tables
    if (interpreter.length) {
        FileTypeEnum ft = filetype_from_interpreter(interpreter);
        if (ft != NONE) {
            return builtin_filetype_names[ft];
        }
    }

    if (base.length) {
        FileTypeEnum ft = filetype_from_basename(base);
        if (ft != NONE) {
            return builtin_filetype_names[ft];
        }
    }

    if (line.length) {
        FileTypeEnum ft = filetype_from_signature(line);
        if (ft != NONE) {
            return builtin_filetype_names[ft];
        }
    }

    if (ext.length) {
        FileTypeEnum ft = filetype_from_extension(ext);
        if (ft != NONE) {
            return builtin_filetype_names[ft];
        }
    }

    if (strview_has_prefix(&path, "/etc/default/")) {
        return builtin_filetype_names[SH];
    } else if (strview_has_prefix(&path, "/etc/nginx/")) {
        return builtin_filetype_names[NGINX];
    }

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
        } else if (
            strview_has_prefix(&path, "/etc/")
            || strview_has_prefix(&path, "/usr/share/")
            || strview_has_prefix(&path, "/usr/local/share/")
        ) {
            return builtin_filetype_names[CONFIG];
        }
    }

    return NULL;
}

bool is_ft(const char *name)
{
    if (BSEARCH(name, builtin_filetype_names, (CompareFunction)strcmp)) {
        return true;
    }

    for (size_t i = 0, n = filetypes.count; i < n; i++) {
        const UserFileTypeEntry *ft = filetypes.ptrs[i];
        if (streq(ft->name, name)) {
            return true;
        }
    }

    return false;
}

void collect_ft(const char *prefix)
{
    for (size_t i = 0; i < ARRAY_COUNT(builtin_filetype_names); i++) {
        const char *name = builtin_filetype_names[i];
        if (str_has_prefix(name, prefix)) {
            add_completion(xstrdup(name));
        }
    }
    for (size_t i = 0, n = filetypes.count; i < n; i++) {
        const UserFileTypeEntry *ft = filetypes.ptrs[i];
        const char *name = ft->name;
        if (str_has_prefix(name, prefix)) {
            add_completion(xstrdup(name));
        }
    }
}

static const char *ft_get_str(const UserFileTypeEntry *ft)
{
    return ft_uses_regex(ft->type) ? ft->u.regexp->str : ft->u.str->str;
}

String dump_ft(void)
{
    static const char flags[][4] = {
        [FT_EXTENSION] = "",
        [FT_FILENAME] = "-f ",
        [FT_CONTENT] = "-c ",
        [FT_INTERPRETER] = "-i ",
        [FT_BASENAME] = "-b ",
    };

    String s = string_new(4096);
    for (size_t i = 0, n = filetypes.count; i < n; i++) {
        const UserFileTypeEntry *ft = filetypes.ptrs[i];
        BUG_ON(ft->type >= ARRAY_COUNT(flags));
        string_append_literal(&s, "ft ");
        string_append_cstring(&s, flags[ft->type]);
        if (unlikely(ft->name[0] == '-')) {
            string_append_cstring(&s, "-- ");
        }
        string_append_escaped_arg(&s, ft->name, true);
        string_append_byte(&s, ' ');
        string_append_escaped_arg(&s, ft_get_str(ft), true);
        string_append_byte(&s, '\n');
    }
    return s;
}
