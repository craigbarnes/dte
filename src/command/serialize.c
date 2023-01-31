#include <string.h>
#include "serialize.h"
#include "util/ascii.h"
#include "util/numtostr.h"

void string_append_escaped_arg_sv(String *s, StringView arg, bool escape_tilde)
{
    static const char escmap[] = {
        [0x07] = 'a', [0x08] = 'b',
        [0x09] = 't', [0x0A] = 'n',
        [0x0B] = 'v', [0x0C] = 'f',
        [0x0D] = 'r', [0x1B] = 'e',
        ['"']  = '"', ['\\'] = '\\',
    };

    if (arg.length == 0) {
        string_append_literal(s, "''");
        return;
    }

    bool has_tilde_slash_prefix = strview_has_prefix(&arg, "~/");
    if (has_tilde_slash_prefix && !escape_tilde) {
        // Print "~/" and skip past it, so it doesn't get quoted
        string_append_literal(s, "~/");
        strview_remove_prefix(&arg, 2);
    }

    bool squote = false;
    for (size_t i = 0, n = arg.length; i < n; i++) {
        const unsigned char c = arg.data[i];
        switch (c) {
        case ' ':
        case '"':
        case '$':
        case ';':
        case '\\':
            squote = true;
            continue;
        case '\'':
            goto dquote;
        }
        if (ascii_iscntrl(c)) {
            goto dquote;
        }
    }

    if (squote) {
        string_append_byte(s, '\'');
        string_append_strview(s, &arg);
        string_append_byte(s, '\'');
    } else {
        if (has_tilde_slash_prefix && escape_tilde) {
            string_append_byte(s, '\\');
        }
        string_append_strview(s, &arg);
    }
    return;

dquote:
    string_append_byte(s, '"');
    for (size_t i = 0, n = arg.length; i < n; i++) {
        unsigned char ch = arg.data[i];
        if (unlikely(ch < sizeof(escmap) && escmap[ch])) {
            string_append_byte(s, '\\');
            ch = escmap[ch];
        } else if (unlikely(ascii_iscntrl(ch))) {
            string_append_literal(s, "\\x");
            string_append_byte(s, hextab_upper[(ch >> 4) & 15]);
            ch = hextab_upper[ch & 15];
        }
        string_append_byte(s, ch);
    }
    string_append_byte(s, '"');
}
