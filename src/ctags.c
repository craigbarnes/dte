#include <stdlib.h>
#include "ctags.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/str-util.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xstring.h"

// Convert an ex(1) style pattern from a tags(5) file to a basic POSIX
// regex ("BRE"), so that it can be compiled with regcomp(3)
static size_t regex_from_ex_pattern (
    const char *ex_str,
    size_t len,
    char **regex_str // out param
) {
    BUG_ON(len == 0);
    const char open_delim = ex_str[0];
    BUG_ON(open_delim != '/' && open_delim != '?');
    char *buf = xmalloc(xmul(2, len));

    // The pattern isn't a real regex; special chars need to be escaped
    for (size_t i = 1, j = 0; i < len; i++) {
        char c = ex_str[i];
        if (c == '\0') {
            break;
        } else if (c == '\\') {
            if (unlikely(++i >= len)) {
                break;
            }
            c = ex_str[i];
            if (c == '\\') {
                // Escape "\\" as "\\" (any other "\x" becomes just "x")
                buf[j++] = '\\';
            }
        } else if (c == '*' || c == '[' || c == ']') {
            buf[j++] = '\\';
        } else if (c == open_delim) {
            buf[j] = '\0';
            *regex_str = buf;
            return i + 1;
        }
        buf[j++] = c;
    }

    // End of string reached without a matching end delimiter; invalid input
    free(buf);
    return 0;
}

static size_t parse_ex_cmd(Tag *tag, const char *buf, size_t size)
{
    if (unlikely(size == 0)) {
        return 0;
    }

    size_t n;
    if (buf[0] == '/' || buf[0] == '?') {
        n = regex_from_ex_pattern(buf, size, &tag->pattern);
    } else {
        n = buf_parse_ulong(buf, size, &tag->lineno);
    }

    if (n == 0) {
        return 0;
    }

    bool trailing_comment = (n + 1 < size) && mem_equal(buf + n, ";\"", 2);
    return n + (trailing_comment ? 2 : 0);
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
    size_t *posp, // in-out param
    const StringView *prefix,
    bool exact,
    Tag *tag // out param
) {
    const char *p = prefix->data;
    size_t plen = prefix->length;
    for (size_t pos = *posp; pos < buf_len; ) {
        StringView line = buf_slice_next_line(buf, &pos, buf_len);
        if (
            line.length > 0 // Line is non-empty
            && line.data[0] != '!' // and not a comment
            && strview_has_strn_prefix(&line, p, plen) // and starts with `prefix`
            && (!exact || line.data[plen] == '\t') // and matches `prefix` exactly, if applicable
            && parse_ctags_line(tag, line.data, line.length) // and is a valid tags(5) entry
        ) {
            // Advance the position; `tag` has been filled by parse_ctags_line()
            *posp = pos;
            return true;
        }
    }

    // No matching tags remaining
    return false;
}

// NOTE: tag itself is not freed
void free_tag(Tag *tag)
{
    free(tag->pattern);
}
