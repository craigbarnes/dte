#include "command.h"
#include "debug.h"
#include "editor.h"
#include "env.h"
#include "util/ascii.h"
#include "util/unicode.h"
#include "util/xmalloc.h"

static size_t parse_sq(const char *cmd, size_t len, String *buf)
{
    size_t pos = 0;
    char ch = '\0';
    while (pos < len) {
        ch = cmd[pos];
        if (ch == '\'') {
            break;
        }
        pos++;
    }
    string_append_buf(buf, cmd, pos);
    if (ch == '\'') {
        pos++;
    }
    return pos;
}

static size_t unicode_escape(const char *str, size_t count, String *buf)
{
    if (count == 0) {
        return 0;
    }

    CodePoint u = 0;
    size_t i;
    for (i = 0; i < count; i++) {
        int x = hex_decode(str[i]);
        if (x < 0) {
            break;
        }
        u = u << 4 | x;
    }
    if (u_is_unicode(u)) {
        string_append_codepoint(buf, u);
    }
    return i;
}

static size_t parse_dq(const char *cmd, size_t len, String *buf)
{
    size_t pos = 0;
    while (pos < len) {
        unsigned char ch = cmd[pos++];

        if (ch == '"') {
            break;
        }

        if (ch == '\\' && pos < len) {
            ch = cmd[pos++];
            switch (ch) {
            case 'a': ch = '\a'; break;
            case 'b': ch = '\b'; break;
            case 'e': ch = '\033'; break;
            case 'f': ch = '\f'; break;
            case 'n': ch = '\n'; break;
            case 'r': ch = '\r'; break;
            case 't': ch = '\t'; break;
            case 'v': ch = '\v'; break;
            case '\\':
            case '"':
                break;
            case 'x':
                if (pos < len) {
                    const int x1 = hex_decode(cmd[pos]);
                    if (x1 >= 0 && ++pos < len) {
                        const int x2 = hex_decode(cmd[pos]);
                        if (x2 >= 0) {
                            pos++;
                            ch = x1 << 4 | x2;
                            break;
                        }
                    }
                }
                continue;
            case 'u':
                pos += unicode_escape(cmd + pos, MIN(4, len - pos), buf);
                continue;
            case 'U':
                pos += unicode_escape(cmd + pos, MIN(8, len - pos), buf);
                continue;
            default:
                string_append_byte(buf, '\\');
                break;
            }
        }

        string_append_byte(buf, ch);
    }

    return pos;
}

static size_t parse_var(const char *cmd, size_t len, String *buf)
{
    if (len == 0 || !is_alpha_or_underscore(cmd[0])) {
        return 0;
    }

    size_t n = 1;
    while (n < len && is_alnum_or_underscore(cmd[n])) {
        n++;
    }

    char *name = xstrcut(cmd, n);
    char *value;
    if (expand_builtin_env(name, &value)) {
        if (value != NULL) {
            string_append_cstring(buf, value);
            free(value);
        }
    } else {
        const char *val = getenv(name);
        if (val != NULL) {
            string_append_cstring(buf, val);
        }
    }

    free(name);
    return n;
}

char *parse_command_arg(const char *cmd, size_t len, bool tilde)
{
    String buf;
    size_t pos = 0;

    if (tilde && len >= 2 && cmd[0] == '~' && cmd[1] == '/') {
        const size_t home_dir_len = strlen(editor.home_dir);
        buf = string_new(len + home_dir_len);
        string_append_buf(&buf, editor.home_dir, home_dir_len);
        string_append_byte(&buf, '/');
        pos += 2;
    } else {
        buf = string_new(len);
    }

    while (pos < len) {
        char ch = cmd[pos++];
        switch (ch) {
        case '\t':
        case '\n':
        case '\r':
        case ' ':
        case ';':
            goto end;
        case '\'':
            pos += parse_sq(cmd + pos, len - pos, &buf);
            break;
        case '"':
            pos += parse_dq(cmd + pos, len - pos, &buf);
            break;
        case '$':
            pos += parse_var(cmd + pos, len - pos, &buf);
            break;
        case '\\':
            if (pos == len) {
                goto end;
            }
            ch = cmd[pos++];
            // Fallthrough
        default:
            string_append_byte(&buf, ch);
            break;
        }
    }

end:
    return string_steal_cstring(&buf);
}

size_t find_end(const char *cmd, const size_t startpos, CommandParseError *err)
{
    size_t pos = startpos;
    while (1) {
        switch (cmd[pos++]) {
        case '\'':
            while (1) {
                if (cmd[pos] == '\'') {
                    pos++;
                    break;
                }
                if (cmd[pos] == '\0') {
                    *err = CMDERR_UNCLOSED_SQUOTE;
                    return 0;
                }
                pos++;
            }
            break;
        case '"':
            while (1) {
                if (cmd[pos] == '"') {
                    pos++;
                    break;
                }
                if (cmd[pos] == '\0') {
                    *err = CMDERR_UNCLOSED_DQUOTE;
                    return 0;
                }
                if (cmd[pos++] == '\\') {
                    if (cmd[pos] == '\0') {
                        goto unexpected_eof;
                    }
                    pos++;
                }
            }
            break;
        case '\\':
            if (cmd[pos] == '\0') {
                goto unexpected_eof;
            }
            pos++;
            break;
        case '\0':
        case '\t':
        case '\n':
        case '\r':
        case ' ':
        case ';':
            *err = CMDERR_NONE;
            return pos - 1;
        }
    }
    BUG("Unexpected break of outer loop");
unexpected_eof:
    *err = CMDERR_UNEXPECTED_EOF;
    return 0;
}

CommandParseError parse_commands(PointerArray *array, const char *cmd)
{
    size_t pos = 0;
    while (1) {
        while (ascii_isspace(cmd[pos])) {
            pos++;
        }

        if (cmd[pos] == '\0') {
            break;
        }

        if (cmd[pos] == ';') {
            ptr_array_append(array, NULL);
            pos++;
            continue;
        }

        CommandParseError err;
        size_t end = find_end(cmd, pos, &err);
        if (err != CMDERR_NONE) {
            return err;
        }

        ptr_array_append(array, parse_command_arg(cmd + pos, end - pos, true));
        pos = end;
    }
    ptr_array_append(array, NULL);
    return CMDERR_NONE;
}

const char *command_parse_error_to_string(CommandParseError err)
{
    static const char error_strings[][16] = {
        [CMDERR_UNCLOSED_SQUOTE] = "unclosed '",
        [CMDERR_UNCLOSED_DQUOTE] = "unclosed \"",
        [CMDERR_UNEXPECTED_EOF] = "unexpected EOF",
    };
    BUG_ON(err <= CMDERR_NONE);
    BUG_ON(err >= ARRAY_COUNT(error_strings));
    return error_strings[err];
}

void string_append_escaped_arg(String *s, const char *arg, bool escape_tilde)
{
    static const char escmap[32] = {
        [0x07] = 'a', [0x08] = 'b',
        [0x09] = 't', [0x0A] = 'n',
        [0x0B] = 'v', [0x0C] = 'f',
        [0x0D] = 'r', [0x1B] = 'e',
    };

    size_t len = strlen(arg);
    if (len == 0) {
        string_append_literal(s, "''");
        return;
    }

    bool squote = false;
    for (size_t i = 0; i < len; i++) {
        char c = arg[i];
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
        string_append_buf(s, arg, len);
        string_append_byte(s, '\'');
    } else {
        if (arg[0] == '~' && arg[1] == '/' && escape_tilde) {
            string_append_byte(s, '\\');
        }
        string_append_buf(s, arg, len);
    }
    return;

dquote:
    string_append_byte(s, '"');
    for (size_t i = 0; i < len; i++) {
        const unsigned char ch = arg[i];
        if (ch < sizeof(escmap)) {
            if (escmap[ch]) {
                string_append_byte(s, '\\');
                string_append_byte(s, escmap[ch]);
            } else {
                string_sprintf(s, "\\x%02hhX", ch);
            }
        } else if (ch == 0x7F) {
            string_append_literal(s, "\\x7F");
        } else if (ch == '"' || ch == '\\') {
            string_append_byte(s, '\\');
            string_append_byte(s, ch);
        } else {
            string_append_byte(s, ch);
        }
    }
    string_append_byte(s, '"');
}

char *escape_command_arg(const char *arg, bool escape_tilde)
{
    String buf = STRING_INIT;
    string_append_escaped_arg(&buf, arg, escape_tilde);
    return string_steal_cstring(&buf);
}
