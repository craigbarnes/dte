#include <stdlib.h>
#include <string.h>
#include "ctags.h"
#include "common.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"

static size_t parse_excmd(Tag *t, const char *buf, size_t size)
{
    const char ch = *buf;
    if (ch == '/' || ch == '?') {
        // The search pattern is not a real regular expression.
        // Need to escape special characters.
        char *pattern = xnew(char, size * 2);
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

    unsigned long line;
    size_t i = buf_parse_ulong(buf, size, &line);
    if (i == 0) {
        return 0;
    }

    if (i + 1 < size && buf[i] == ';' && buf[i + 1] == '"') {
        i += 2;
    }

    t->line = line;
    return i;
}

static bool parse_line(Tag *t, const char *buf, size_t size)
{
    memzero(t);
    const char *end = memchr(buf, '\t', size);
    if (!end) {
        goto error;
    }

    size_t len = end - buf;
    t->name = xstrcut(buf, len);

    size_t si = len + 1;
    if (si >= size) {
        goto error;
    }

    end = memchr(buf + si, '\t', size - si);
    len = end - buf - si;
    t->filename = xstrslice(buf, si, si + len);

    si += len + 1;
    if (si >= size) {
        goto error;
    }

    // excmd can contain tabs
    len = parse_excmd(t, buf + si, size - si);
    if (!len) {
        goto error;
    }

    si += len;
    if (si == size) {
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
    if (buf[si] != '\t') {
        goto error;
    }

    si++;
    while (si < size) {
        size_t ei = si;

        while (ei < size && buf[ei] != '\t') {
            ei++;
        }

        len = ei - si;
        if (len == 1) {
            t->kind = buf[si];
        } else if (len == 5 && !memcmp(buf + si, "file:", 5)) {
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
    size_t prefix_len = strlen(prefix);
    size_t pos = *posp;

    while (pos < tf->size) {
        size_t len = tf->size - pos;
        char *line = tf->buf + pos;
        char *end = memchr(line, '\n', len);

        if (end) {
            len = end - line;
        }
        pos += len + 1;

        if (!len || line[0] == '!') {
            continue;
        }

        if (len <= prefix_len || memcmp(line, prefix, prefix_len) != 0) {
            continue;
        }

        if (exact && line[prefix_len] != '\t') {
            continue;
        }

        if (!parse_line(t, line, len)) {
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
    free(t->name);
    free(t->filename);
    free(t->pattern);
    free(t->member);
    free(t->typeref);
}
