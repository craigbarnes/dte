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

// Filetypes dynamically added via the `ft` command.
// Not grouped by name to make it possible to order them freely.
typedef struct {
    FileDetectionType type : 8;
    unsigned int name_len : 8;
    unsigned int str_len : 16;
    char data[]; // Contains name followed by str (both null-terminated)
} UserFileTypeEntry;

static PointerArray filetypes = PTR_ARRAY_INIT;

static const char *ft_get_name(const UserFileTypeEntry *ft)
{
    return ft->data;
}

static const char *ft_get_str(const UserFileTypeEntry *ft)
{
    return ft->data + ft->name_len + 1;
}

void add_filetype(const char *name, const char *str, FileDetectionType type)
{
    const size_t name_len = strlen(name);
    const size_t str_len = strlen(str);
    if (unlikely(name_len > 0xFF || str_len > 0xFFFF)) {
        error_msg("ft argument exceeds maximum length");
        return;
    }

    if (type == FT_CONTENT || type == FT_FILENAME) {
        if (!regexp_is_valid(str, REG_NEWLINE)) {
            return;
        }
    }

    const size_t data_len = name_len + str_len + 2;
    UserFileTypeEntry *ft = xmalloc(sizeof(*ft) + data_len);
    ft->type = type;
    ft->name_len = name_len;
    ft->str_len = str_len;
    memcpy(ft->data, name, name_len + 1);
    memcpy(ft->data + name_len + 1, str, str_len + 1);
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
static StringView get_interpreter(const StringView line)
{
    StringView sv = STRING_VIEW_INIT;
    if (line.length < 4 || line.data[0] != '#' || line.data[1] != '!') {
        return sv;
    }

    static const char pat[] = "^#! */.*(/env +|/)([a-zA-Z0-9_-]+)[0-9.]*( |$)";
    static regex_t re;
    static bool compiled;
    if (!compiled) {
        compiled = regexp_compile(&re, pat, REG_NEWLINE);
        BUG_ON(!compiled);
        BUG_ON(re.re_nsub < 2);
    }

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

static bool ft_str_match(const UserFileTypeEntry *ft, const StringView sv)
{
    const char *str = ft_get_str(ft);
    const size_t len = (size_t)ft->str_len;
    return sv.length > 0 && strview_equal_strn(&sv, str, len);
}

static bool ft_regex_match(const UserFileTypeEntry *ft, const StringView sv)
{
    const char *str = ft_get_str(ft);
    return sv.length > 0 && regexp_match_nosub(str, &sv);
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
        return ft_get_name(ft);
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
        if (streq(ft_get_name(ft), name)) {
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
        const char *name = ft_get_name(ft);
        if (str_has_prefix(name, prefix)) {
            add_completion(xstrdup(name));
        }
    }
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
        const char *name = ft_get_name(ft);
        FileDetectionType type = ft->type;
        BUG_ON(type >= ARRAY_COUNT(flags));
        string_append_literal(&s, "ft ");
        string_append_cstring(&s, flags[type]);
        if (unlikely(name[0] == '-')) {
            string_append_cstring(&s, "-- ");
        }
        string_append_escaped_arg(&s, name, true);
        string_append_byte(&s, ' ');
        string_append_escaped_arg(&s, ft_get_str(ft), true);
        string_append_byte(&s, '\n');
    }
    return s;
}
