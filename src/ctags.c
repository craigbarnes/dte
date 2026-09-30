#include <stdlib.h>
#include "ctags.h"
#include "util/arith.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/str-util.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"

// Convert an ex(1) style pattern from a tags(5) file to a basic POSIX
// regex ("BRE"), so that it can be compiled with regcomp(3)
static size_t regex_from_ex_pattern(StringView ex, char **regex_str)
{
    BUG_ON(ex.length == 0);
    const char open_delim = ex.data[0];
    BUG_ON(open_delim != '/' && open_delim != '?');
    char *buf = xmalloc(xmul(2, ex.length));

    // The pattern isn't a real regex; special chars need to be escaped
    for (size_t i = 1, j = 0; i < ex.length; i++) {
        char c = ex.data[i];
        if (c == '\0') {
            break;
        } else if (c == '\\') {
            if (unlikely(++i >= ex.length)) {
                break;
            }
            c = ex.data[i];
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

static size_t parse_ex_cmd(Tag *tag, StringView cmd)
{
    if (unlikely(cmd.length == 0)) {
        return 0;
    }

    size_t n;
    if (strview_has_either_prefix(cmd, "/", "?")) {
        n = regex_from_ex_pattern(cmd, &tag->pattern);
    } else {
        n = buf_parse_ulong(cmd, &tag->lineno);
    }

    if (n == 0) {
        return 0;
    }

    strview_remove_prefix(&cmd, n);
    StringView delim = strview(";\"");
    bool trailing_comment = strview_has_sv_prefix(cmd, delim);
    return n + (trailing_comment ? delim.length : 0);
}

bool parse_ctags_line(Tag *tag, StringView line)
{
    size_t pos = 0;
    *tag = (Tag){.name = get_delim(line, &pos, '\t')};
    if (tag->name.length == 0 || pos >= line.length) {
        return false;
    }

    tag->filename = get_delim(line, &pos, '\t');
    if (tag->filename.length == 0 || pos >= line.length) {
        return false;
    }

    size_t len = parse_ex_cmd(tag, strview_suffix(line, pos));
    if (len == 0) {
        BUG_ON(tag->pattern);
        return false;
    }

    pos += len;
    if (pos >= line.length) {
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
    if (line.data[pos++] != '\t') {
        // free `pattern` allocated by parse_ex_cmd()
        free_tag(tag);
        tag->pattern = NULL;
        return false;
    }

    while (pos < line.length) {
        StringView field = get_delim(line, &pos, '\t');
        if (field.length == 1 && ascii_isalpha(field.data[0])) {
            tag->kind = field.data[0];
        } else if (strview_equal_cstring(field, "file:")) {
            tag->local = true;
        }
        // TODO: struct/union/typeref
    }

    return true;
}

bool next_tag (
    StringView text, // Tag file contents
    size_t *posp, // Current position within `text` [in-out param]
    StringView prefix,
    bool exact,
    Tag *tag // [out param]
) {
    for (size_t pos = *posp; pos < text.length; ) {
        StringView line = buf_slice_next_line(text.data, &pos, text.length);
        if (
            line.length > 0 // Line is non-empty
            && line.data[0] != '!' // and not a comment
            && strview_has_sv_prefix(line, prefix) // and starts with `prefix`
            && (!exact || line.data[prefix.length] == '\t') // and matches `prefix` exactly, if applicable
            && parse_ctags_line(tag, line) // and is a valid tags(5) entry
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
