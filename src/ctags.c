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
    MEMZERO(t);
    const char *end = memchr(line, '\t', line_len);
    if (!end) {
        goto error;
    }

    size_t len = end - line;
    t->name = string_view(line, len);

    size_t si = len + 1;
    if (si >= line_len) {
        goto error;
    }

    end = memchr(line + si, '\t', line_len - si);
    len = end - line - si;
    t->filename = string_view(line + si, len);

    si += len + 1;
    if (si >= line_len) {
        goto error;
    }

    // excmd can contain tabs
    len = parse_excmd(t, line + si, line_len - si);
    if (len == 0) {
        goto error;
    }

    si += len;
    if (si == line_len) {
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
    if (line[si] != '\t') {
        goto error;
    }

    si++;
    while (si < line_len) {
        size_t ei = si;
        while (ei < line_len && line[ei] != '\t') {
            ei++;
        }
        len = ei - si;
        if (len == 1) {
            t->kind = line[si];
        } else if (len == 5 && mem_equal(line + si, "file:", 5)) {
            t->local = true;
        }
        // FIXME: struct/union/typeref
        si = ei + 1;
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
