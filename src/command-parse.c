#include "command.h"
#include "debug.h"
#include "editor.h"
#include "env.h"
#include "error.h"
#include "util/ascii.h"
#include "util/ptr-array.h"
#include "util/str-util.h"
#include "util/string.h"
#include "util/utf8.h"
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
                    *err = CMDERR_UNCLOSED_SINGLE_QUOTE;
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
                    *err = CMDERR_UNCLOSED_DOUBLE_QUOTE;
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
            return pos - 1;
        }
    }
    BUG("Unexpected break of outer loop");
unexpected_eof:
    *err = CMDERR_UNEXPECTED_EOF;
    return 0;
}

bool parse_commands(PointerArray *array, const char *cmd, CommandParseError *err)
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

        size_t end = find_end(cmd, pos, err);
        if (*err != CMDERR_NONE) {
            return false;
        }

        ptr_array_append(array, parse_command_arg(cmd + pos, end - pos, true));
        pos = end;
    }
    ptr_array_append(array, NULL);
    return true;
}

const char *command_parse_error_to_string(CommandParseError err)
{
    static const char error_strings[][16] = {
        [CMDERR_UNCLOSED_SINGLE_QUOTE] = "Missing '",
        [CMDERR_UNCLOSED_DOUBLE_QUOTE] = "Missing \"",
        [CMDERR_UNEXPECTED_EOF] = "Unexpected EOF",
    };
    BUG_ON(err <= CMDERR_NONE);
    BUG_ON(err >= ARRAY_COUNT(error_strings));
    return error_strings[err];
}
