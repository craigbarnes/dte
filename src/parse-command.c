#include "command.h"
#include "common.h"
#include "editor.h"
#include "env.h"
#include "error.h"
#include "util/ascii.h"
#include "util/ptr-array.h"
#include "util/string.h"
#include "util/uchar.h"
#include "util/xmalloc.h"

static void parse_sq(const char *cmd, size_t *posp, String *buf)
{
    const size_t start_pos = *posp;
    size_t pos = start_pos;
    char ch;
    while (1) {
        ch = cmd[pos];
        if (ch == '\'' || ch == '\0') {
            break;
        }
        pos++;
    }
    string_add_buf(buf, cmd + start_pos, pos - start_pos);
    if (ch == '\'') {
        pos++;
    }
    *posp = pos;
}

static size_t unicode_escape(const char *str, size_t count, String *buf)
{
    CodePoint u = 0;
    size_t i;
    for (i = 0; i < count && str[i]; i++) {
        int x = hex_decode(str[i]);
        if (x < 0) {
            break;
        }
        u = u << 4 | x;
    }
    if (u_is_unicode(u)) {
        string_add_ch(buf, u);
    }
    return i;
}

static void parse_dq(const char *cmd, size_t *posp, String *buf)
{
    size_t pos = *posp;

    while (cmd[pos]) {
        unsigned char ch = cmd[pos++];

        if (ch == '"') {
            break;
        }

        if (ch == '\\' && cmd[pos]) {
            ch = cmd[pos++];
            switch (ch) {
            case 'a': ch = '\a'; break;
            case 'b': ch = '\b'; break;
            case 'f': ch = '\f'; break;
            case 'n': ch = '\n'; break;
            case 'r': ch = '\r'; break;
            case 't': ch = '\t'; break;
            case 'v': ch = '\v'; break;
            case '\\':
            case '"':
                break;
            case 'x':
                if (cmd[pos]) {
                    int x1, x2;
                    x1 = hex_decode(cmd[pos]);
                    if (x1 >= 0 && cmd[++pos]) {
                        x2 = hex_decode(cmd[pos]);
                        if (x2 >= 0) {
                            pos++;
                            ch = x1 << 4 | x2;
                            break;
                        }
                    }
                }
                continue;
            case 'u':
                pos += unicode_escape(cmd + pos, 4, buf);
                continue;
            case 'U':
                pos += unicode_escape(cmd + pos, 8, buf);
                continue;
            default:
                string_add_byte(buf, '\\');
                break;
            }
        }

        string_add_byte(buf, ch);
    }

    *posp = pos;
}

static void parse_var(const char *cmd, size_t *posp, String *buf)
{
    size_t si = *posp;
    size_t ei = si;

    const char first_char = cmd[ei];
    if (!is_alpha_or_underscore(first_char)) {
        return;
    }
    ei++;

    while (1) {
        if (is_alnum_or_underscore(cmd[ei])) {
            ei++;
            continue;
        }
        break;
    }
    *posp = ei;

    char *name = xstrslice(cmd, si, ei);
    char *value = expand_builtin_env(name);
    if (value != NULL) {
        string_add_str(buf, value);
        free(value);
    } else {
        const char *val = getenv(name);
        if (val != NULL) {
            string_add_str(buf, val);
        }
    }
    free(name);
}

char *parse_command_arg(const char *cmd, bool tilde)
{
    String buf = STRING_INIT;
    size_t pos = 0;

    if (tilde && cmd[pos] == '~' && cmd[pos + 1] == '/') {
        string_add_str(&buf, editor.home_dir);
        pos++;
    }

    while (1) {
        char ch = cmd[pos++];
        switch (ch) {
        case '\0':
        case '\t':
        case '\n':
        case '\r':
        case ' ':
        case ';':
            goto end;
        case '\'':
            parse_sq(cmd, &pos, &buf);
            break;
        case '"':
            parse_dq(cmd, &pos, &buf);
            break;
        case '$':
            parse_var(cmd, &pos, &buf);
            break;
        case '\\':
            ch = cmd[pos++];
            if (ch == '\0') {
                goto end;
            }
            // Fallthrough
        default:
            string_add_byte(&buf, ch);
            break;
        }
    }

end:
    return string_steal_cstring(&buf);
}

size_t find_end(const char *cmd, const size_t startpos, Error **err)
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
                    *err = error_create("Missing '");
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
                    *err = error_create("Missing \"");
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
    *err = error_create("Unexpected EOF");
    return 0;
}

bool parse_commands(PointerArray *array, const char *cmd, Error **err)
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
            ptr_array_add(array, NULL);
            pos++;
            continue;
        }

        size_t end = find_end(cmd, pos, err);
        if (*err != NULL) {
            return false;
        }

        ptr_array_add(array, parse_command_arg(cmd + pos, true));
        pos = end;
    }
    ptr_array_add(array, NULL);
    return true;
}

char **copy_string_array(char **src, size_t count)
{
    char **dst = xnew(char *, count + 1);
    for (size_t i = 0; i < count; i++) {
        dst[i] = xstrdup(src[i]);
    }
    dst[count] = NULL;
    return dst;
}
