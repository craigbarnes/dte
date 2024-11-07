#include <string.h>
#include "serialize.h"
#include "util/ascii.h"
#include "util/numtostr.h"

typedef enum {
    NO_QUOTE,
    SINGLE_QUOTE,
    DOUBLE_QUOTE,
} SerializeType;

static const char escmap[] = {
    [0x07] = 'a',
    [0x08] = 'b',
    [0x09] = 't',
    [0x0A] = 'n',
    [0x0B] = 'v',
    [0x0C] = 'f',
    [0x0D] = 'r',
    [0x1B] = 'e',
    ['"']  = '"',
    ['\\'] = '\\',
};

static void string_append_dquoted_arg(String *s, StringView arg)
{
    string_append_byte(s, '"');

    for (size_t i = 0, n = arg.length; i < n; i++) {
        char *buf = string_reserve_space(s, 4);
        size_t pos = 0;
        unsigned char ch = arg.data[i];
        if (unlikely(ch < sizeof(escmap) && escmap[ch])) {
            buf[pos++] = '\\';
            ch = escmap[ch];
        } else if (unlikely(ascii_iscntrl(ch))) {
            pos += copyliteral(buf + pos, "\\x");
            buf[pos++] = hextab_upper[(ch >> 4) & 0xF];
            ch = hextab_upper[ch & 0xF];
        }
        buf[pos++] = ch;
        s->len += pos;
    }

    string_append_byte(s, '"');
}

static SerializeType get_serialize_type(const unsigned char *arg, size_t n)
{
    SerializeType type = NO_QUOTE;
    for (size_t i = 0; i < n; i++) {
        const unsigned char c = arg[i];
        if (c == '\'' || c < 0x20 || c == 0x7F) {
            return DOUBLE_QUOTE;
        }
        if (c == ' ' || c == '"' || c == '$' || c == ';' || c == '\\') {
            type = SINGLE_QUOTE;
        }
    }
    return type;
}

// Append `arg` to `s`, escaping dterc(5) special characters and/or
// quoting as appropriate, so that the appended string round-trips
// back to `arg` when passed to parse_command_arg(). If escape_tilde
// is true, any leading "~/" substring will be escaped, so as to not
// expand to $HOME/.
void string_append_escaped_arg_sv(String *s, StringView arg, bool escape_tilde)
{
    char *buf = string_reserve_space(s, arg.length + STRLEN("~/''"));
    if (arg.length == 0) {
        s->len += copyliteral(buf, "''");
        return;
    }

    bool has_tilde_slash_prefix = strview_has_prefix(&arg, "~/");
    if (has_tilde_slash_prefix && !escape_tilde) {
        // Print "~/" and skip past it, so it doesn't get quoted
        size_t skip = copyliteral(buf, "~/");
        buf += skip;
        s->len += skip;
        strview_remove_prefix(&arg, skip);
    }

    SerializeType type = get_serialize_type(arg.data, arg.length);
    if (type == DOUBLE_QUOTE) {
        string_append_dquoted_arg(s, arg);
        return;
    }

    size_t n = 0;
    if (type == SINGLE_QUOTE) {
        buf[n++] = '\'';
    } else if (has_tilde_slash_prefix && escape_tilde) {
        buf[n++] = '\\';
    }
    n += copystrn(buf + n, arg.data, arg.length);
    if (type == SINGLE_QUOTE) {
        buf[n++] = '\'';
    }
    s->len += n;
}
