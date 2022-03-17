#include <stdlib.h>
#include <string.h>
#include "ctags.h"
#include "util/str-util.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"

static size_t parse_excmd(Tag *t, const char *buf, size_t size)
{
    const char ch = *buf;
    if (ch == '/' || ch == '?') {
        // The search pattern is not a real regular expression.
        // Need to escape special characters.
        char *pattern = xmalloc(size * 2);
        for (size_t i = 1, j = 0; i < size; i++) {
            if (buf[i] == '\\' && i + 1 < size) {
                i++;
                if (buf[i] == '\\') {
                    pattern[j++] = '\\';
                }
                pattern[j++] = buf[i];
                continue;
            }
            if (buf[i] == ch) {
                if (i + 2 < size && buf[i + 1] == ';' && buf[i + 2] == '"') {
                    i += 2;
                }
                pattern[j] = '\0';
                t->pattern = pattern;
                return i + 1;
            }
            switch (buf[i]) {
            case '*':
            case '[':
            case ']':
                pattern[j++] = '\\';
                break;
            }
            pattern[j++] = buf[i];
        }
        free(pattern);
        return 0;
    }

    unsigned long lineno;
    size_t i = buf_parse_ulong(buf, size, &lineno);
    if (i == 0) {
        return 0;
    }

    if (i + 1 < size && buf[i] == ';' && buf[i + 1] == '"') {
        i += 2;
    }

    t->lineno = lineno;
    return i;
}

static bool parse_line(Tag *t, const char *line, size_t line_len)
{
    size_t pos = 0;
    MEMZERO(t);
    t->name = get_delim(line, &pos, line_len, '\t');
    if (t->name.length == 0 || pos >= line_len) {
        goto error;
    }

    t->filename = get_delim(line, &pos, line_len, '\t');
    if (t->filename.length == 0 || pos >= line_len) {
        goto error;
    }

    // excmd can contain tabs
    size_t len = parse_excmd(t, line + pos, line_len - pos);
    if (len == 0) {
        goto error;
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
        goto error;
    }

    while (pos < line_len) {
        StringView field = get_delim(line, &pos, line_len, '\t');
        if (field.length == 1 && ascii_isalpha(field.data[0])) {
            t->kind = field.data[0];
        } else if (strview_equal_cstring(&field, "file:")) {
            t->local = true;
        }
        // TODO: struct/union/typeref
    }

    return true;

error:
    free_tag(t);
    return false;
}

bool next_tag (
    const TagFile *tf,
    size_t *posp,
    const char *prefix,
    bool exact,
    Tag *t
) {
    size_t pflen = strlen(prefix);
    for (size_t pos = *posp, size = tf->size; pos < size; ) {
        StringView line = buf_slice_next_line(tf->buf, &pos, size);
        if (line.length == 0 || line.data[0] == '!') {
            continue;
        }
        if (line.length <= pflen || !mem_equal(line.data, prefix, pflen)) {
            continue;
        }
        if (exact && line.data[pflen] != '\t') {
            continue;
        }
        if (!parse_line(t, line.data, line.length)) {
            continue;
        }
        *posp = pos;
        return true;
    }
    return false;
}

// NOTE: t itself is not freed
void free_tag(Tag *t)
{
    free(t->pattern);
}
