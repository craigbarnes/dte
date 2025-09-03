#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"
#include "trace.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/strtonum.h"
#include "util/unicode.h"
#include "util/xmalloc.h"
#include "util/xstring.h"

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

static size_t expand_var (
    const CommandRunner *runner,
    const char *cmd,
    size_t len,
    String *buf
) {
    if (!runner->expand_variable || len == 0) {
        return len;
    }

    char *name = xstrcut(cmd, len);
    char *value = runner->expand_variable(runner->e, name);
    free(name);
    if (value) {
        string_append_cstring(buf, value);
        free(value);
    }

    return len;
}

// Handles bracketed variables, e.g. ${varname}
static size_t parse_bracketed_var (
    const CommandRunner *runner,
    const char *cmd,
    size_t len,
    String *buf
) {
    const char *end = memchr(cmd + 1, '}', len - 1);
    if (!end) {
        LOG_WARNING("no end delimiter for bracketed variable: $%.*s", (int)len, cmd);
        return len; // Consume without appending
    }

    size_t var_len = (size_t)(end - cmd) - 1;
    TRACE_CMD("expanding variable: ${%.*s}", (int)var_len, cmd + 1);
    return expand_var(runner, cmd + 1, var_len, buf) + STRLEN("{}");
}

static size_t parse_var (
    const CommandRunner *runner,
    const char *cmd,
    size_t len,
    String *buf
) {
    char ch = len ? cmd[0] : 0;
    if (!is_alpha_or_underscore(ch)) {
        bool bracketed = (ch == '{');
        return bracketed ? parse_bracketed_var(runner, cmd, len, buf) : 0;
    }

    AsciiCharType type_mask = ASCII_ALNUM | ASCII_UNDERSCORE;
    size_t var_len = ascii_type_prefix_length(cmd, len, type_mask);
    TRACE_CMD("expanding variable: $%.*s", (int)var_len, cmd);
    return expand_var(runner, cmd, var_len, buf);
}

// Parse a single dterc(5) argument from `cmd`, stopping when an unquoted
// whitespace or semicolon character is found or when all `len` bytes have
// been processed without encountering such a character. Escape sequences
// and $variables are expanded during processing and the fully expanded
// result is returned as a malloc'd string.
char *parse_command_arg(const CommandRunner *runner, const char *cmd, size_t len)
{
    const StringView *home = runner->home_dir;
    bool expand_ts = (runner->flags & CMDRUNNER_EXPAND_TILDE_SLASH);
    bool tilde_slash = expand_ts && len >= 2 && mem_equal(cmd, "~/", 2);
    String buf = string_new(len + 1 + (tilde_slash ? home->length : 0));
    size_t pos = 0;

    if (tilde_slash) {
        string_append_strview(&buf, *home);
        pos += 1; // Skip past '~' and leave '/' to be handled below
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
CommandParseError parse_commands (
    const CommandRunner *runner,
    PointerArray *array,
    const char *cmd
) {
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
