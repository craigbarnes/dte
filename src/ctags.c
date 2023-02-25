#include <stdlib.h>
#include <string.h>
#include "ctags.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/str-util.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"

static size_t parse_ex_pattern(const char *buf, size_t size, char **escaped)
{
    BUG_ON(size == 0);
    BUG_ON(buf[0] != '/' && buf[0] != '?');

    // The search pattern is not a real regular expression; special characters
    // need to be escaped
    char *pattern = xmalloc(size * 2);
    char open_delim = buf[0];
    for (size_t i = 1, j = 0; i < size; i++) {
        if (unlikely(buf[i] == '\0')) {
            break;
        }
        if (buf[i] == '\\' && i + 1 < size) {
            i++;
            if (buf[i] == '\\') {
                pattern[j++] = '\\';
            }
            pattern[j++] = buf[i];
            continue;
        }
        if (buf[i] == open_delim) {
            pattern[j] = '\0';
            *escaped = pattern;
            return i + 1;
        }
        char c = buf[i];
        if (c == '*' || c == '[' || c == ']') {
            pattern[j++] = '\\';
        }
        pattern[j++] = buf[i];
    }

    free(pattern);
    return 0;
}

static size_t parse_ex_cmd(Tag *tag, const char *buf, size_t size)
{
    if (unlikely(size == 0)) {
        return 0;
    }

    size_t n;
    if (buf[0] == '/' || buf[0] == '?') {
        n = parse_ex_pattern(buf, size, &tag->pattern);
    } else {
        n = buf_parse_ulong(buf, size, &tag->lineno);
    }

    if (n == 0) {
        return 0;
    }

    if (n + 1 < size && buf[n] == ';' && buf[n + 1] == '"') {
        n += 2;
    }

    return n;
}

bool parse_ctags_line(Tag *tag, const char *line, size_t line_len)
{
    size_t pos = 0;
    *tag = (Tag){.name = get_delim(line, &pos, line_len, '\t')};
    if (tag->name.length == 0 || pos >= line_len) {
        return false;
    }

    tag->filename = get_delim(line, &pos, line_len, '\t');
    if (tag->filename.length == 0 || pos >= line_len) {
        return false;
    }

    size_t len = parse_ex_cmd(tag, line + pos, line_len - pos);
    if (len == 0) {
        BUG_ON(tag->pattern);
        return false;
    }

    pos += len;
    if (pos >= line_len) {
        return true;
    }

    /*
     * Extension fields (key:[value]):
     *
     * file:                              visibility limited to this file
     * struct:NAME                        tag is member of struct NAME
     * union:NAME                         tag is member of union NAME
     * typeref:struct:NAME::MEMBER_TYPE   MEMBER_TYPE is type of the tag
     */
    if (line[pos++] != '\t') {
        // free `pattern` allocated by parse_ex_cmd()
        free_tag(tag);
        tag->pattern = NULL;
        return false;
    }

    while (pos < line_len) {
        StringView field = get_delim(line, &pos, line_len, '\t');
        if (field.length == 1 && ascii_isalpha(field.data[0])) {
            tag->kind = field.data[0];
        } else if (strview_equal_cstring(&field, "file:")) {
            tag->local = true;
        }
        // TODO: struct/union/typeref
    }

    return true;
}

bool next_tag (
    const char *buf,
    size_t buf_len,
    size_t *posp,
    const StringView *prefix,
    bool exact,
    Tag *tag
) {
    const char *p = prefix->data;
    size_t plen = prefix->length;
    for (size_t pos = *posp; pos < buf_len; ) {
        StringView line = buf_slice_next_line(buf, &pos, buf_len);
        if (line.length == 0 || line.data[0] == '!') {
            continue;
        }
        if (!strview_has_strn_prefix(&line, p, plen)) {
            continue;
        }
        if (exact && line.data[plen] != '\t') {
            continue;
        }
        if (!parse_ctags_line(tag, line.data, line.length)) {
            continue;
        }
        *posp = pos;
        return true;
    }
    return false;
}

// NOTE: tag itself is not freed
void free_tag(Tag *tag)
{
    free(tag->pattern);
}
