#include <stdlib.h>
#include <string.h>
#include "parse.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/strtonum.h"
#include "util/unicode.h"
#include "util/xmalloc.h"

static size_t parse_sq(const char *cmd, size_t len, String *buf)
{
    const char *end = memchr(cmd, '\'', len);
    size_t pos = end ? (size_t)(end - cmd) : len;
    string_append_buf(buf, cmd, pos);
    return pos + (end ? 1 : 0);
}

static size_t unicode_escape(const char *str, size_t count, String *buf)
{
    // Note: `u` doesn't need to be initialized here, but `gcc -Og`
    // gives a spurious -Wmaybe-uninitialized warning if it's not
    unsigned int u = 0;
    static_assert(sizeof(u) >= 4);
    size_t n = buf_parse_hex_uint(str, count, &u);
    if (likely(n > 0 && u_is_unicode(u))) {
        string_append_codepoint(buf, u);
    }
    return n;
}

static size_t hex_escape(const char *str, size_t count, String *buf)
{
    unsigned int x = 0;
    size_t n = buf_parse_hex_uint(str, count, &x);
    if (likely(n == 2)) {
        string_append_byte(buf, x);
    }
    return n;
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
                pos += hex_escape(cmd + pos, MIN(2, len - pos), buf);
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

static size_t parse_var(const CommandRunner *runner, const char *cmd, size_t len, String *buf)
{
    if (len == 0 || !is_alpha_or_underscore(cmd[0])) {
        return 0;
    }

    size_t n = 1;
    while (n < len && is_alnum_or_underscore(cmd[n])) {
        n++;
    }

    if (runner->expand_variable) {
        char *name = xstrcut(cmd, n);
        char *value = runner->expand_variable(runner->e, name);
        free(name);
        if (value) {
            string_append_cstring(buf, value);
            free(value);
        }
    }

    return n;
}

char *parse_command_arg(const CommandRunner *runner, const char *cmd, size_t len)
{
    String buf;
    size_t pos = 0;

    if (runner->expand_tilde_slash && len >= 2 && cmd[0] == '~' && cmd[1] == '/') {
        buf = string_new(len + runner->home_dir->length);
        string_append_strview(&buf, runner->home_dir);
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
            pos += parse_var(runner, cmd + pos, len - pos, &buf);
            break;
        case '\\':
            if (unlikely(pos == len)) {
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

size_t find_end(const char *cmd, size_t pos, CommandParseError *err)
{
    while (1) {
        switch (cmd[pos++]) {
        case '\'':
            while (1) {
                if (cmd[pos] == '\'') {
                    pos++;
                    break;
                }
                if (unlikely(cmd[pos] == '\0')) {
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
                if (unlikely(cmd[pos] == '\0')) {
                    *err = CMDERR_UNCLOSED_DQUOTE;
                    return 0;
                }
                if (cmd[pos++] == '\\') {
                    if (unlikely(cmd[pos] == '\0')) {
                        *err = CMDERR_UNEXPECTED_EOF;
                        return 0;
                    }
                    pos++;
                }
            }
            break;
        case '\\':
            if (unlikely(cmd[pos] == '\0')) {
                *err = CMDERR_UNEXPECTED_EOF;
                return 0;
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
}

// Note: `array` must be freed, regardless of the return value
CommandParseError parse_commands(const CommandRunner *runner, PointerArray *array, const char *cmd)
{
    for (size_t pos = 0; true; ) {
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

        ptr_array_append(array, parse_command_arg(runner, cmd + pos, end - pos));
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
    BUG_ON(err >= ARRAYLEN(error_strings));
    return error_strings[err];
}
