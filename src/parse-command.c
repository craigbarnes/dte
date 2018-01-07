#include "command.h"
#include "editor.h"
#include "error.h"
#include "str.h"
#include "ptr-array.h"
#include "env.h"
#include "common.h"
#include "uchar.h"

static String arg = STRING_INIT;

static void parse_sq(const char *cmd, size_t *posp)
{
    size_t pos = *posp;

    while (1) {
        if (cmd[pos] == '\'') {
            pos++;
            break;
        }
        if (!cmd[pos]) {
            break;
        }
        string_add_byte(&arg, cmd[pos++]);
    }
    *posp = pos;
}

static size_t unicode_escape(const char *str, size_t count)
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
        string_add_ch(&arg, u);
    }
    return i;
}

static void parse_dq(const char *cmd, size_t *posp)
{
    size_t pos = *posp;

    while (cmd[pos]) {
        char ch = cmd[pos++];

        if (ch == '"') {
            break;
        }

        if (ch == '\\' && cmd[pos]) {
            ch = cmd[pos++];
            switch (ch) {
            case '\\':
            case '"':
                string_add_byte(&arg, ch);
                break;
            case 'a':
                string_add_ch(&arg, '\a');
                break;
            case 'b':
                string_add_ch(&arg, '\b');
                break;
            case 'f':
                string_add_ch(&arg, '\f');
                break;
            case 'n':
                string_add_ch(&arg, '\n');
                break;
            case 'r':
                string_add_ch(&arg, '\r');
                break;
            case 't':
                string_add_ch(&arg, '\t');
                break;
            case 'v':
                string_add_ch(&arg, '\v');
                break;
            case 'x':
                if (cmd[pos]) {
                    int x1, x2;
                    x1 = hex_decode(cmd[pos]);
                    if (x1 >= 0 && cmd[++pos]) {
                        x2 = hex_decode(cmd[pos]);
                        if (x2 >= 0) {
                            pos++;
                            string_add_byte(&arg, x1 << 4 | x2);
                        }
                    }
                }
                break;
            case 'u':
                pos += unicode_escape(cmd + pos, 4);
                break;
            case 'U':
                pos += unicode_escape(cmd + pos, 8);
                break;
            default:
                string_add_ch(&arg, '\\');
                string_add_byte(&arg, ch);
                break;
            }
        } else {
            string_add_byte(&arg, ch);
        }
    }
    *posp = pos;
}

static void parse_var(const char *cmd, size_t *posp)
{
    size_t si = *posp;
    size_t ei = si;
    char *name, *value;

    while (1) {
        char ch = cmd[ei];

        if (ascii_isalpha(ch) || ch == '_') {
            ei++;
            continue;
        }
        if (ei > si && ascii_isdigit(ch)) {
            ei++;
            continue;
        }
        break;
    }
    *posp = ei;
    if (ei == si) {
        return;
    }
    name = xstrslice(cmd, si, ei);
    value = expand_builtin_env(name);
    if (value != NULL) {
        string_add_str(&arg, value);
        free(value);
    } else {
        const char *val = getenv(name);
        if (val != NULL) {
            string_add_str(&arg, val);
        }
    }
    free(name);
}

char *parse_command_arg(const char *cmd, bool tilde)
{
    size_t pos = 0;

    if (tilde && cmd[pos] == '~' && cmd[pos+1] == '/') {
        string_add_str(&arg, editor.home_dir);
        pos++;
    }

    while (1) {
        char ch = cmd[pos];

        if (!ch || ch == ';' || ascii_isspace(ch)) {
            break;
        }

        pos++;
        if (ch == '\'') {
            parse_sq(cmd, &pos);
        } else if (ch == '"') {
            parse_dq(cmd, &pos);
        } else if (ch == '$') {
            parse_var(cmd, &pos);
        } else if (ch == '\\') {
            if (!cmd[pos]) {
                break;
            }
            string_add_byte(&arg, cmd[pos++]);
        } else {
            string_add_byte(&arg, ch);
        }
    }
    return string_steal_cstring(&arg);
}

size_t find_end(const char *cmd, size_t pos, Error **err)
{
    while (1) {
        char ch = cmd[pos];

        if (!ch || ch == ';' || ascii_isspace(ch)) {
            break;
        }

        pos++;
        if (ch == '\'') {
            while (1) {
                if (cmd[pos] == '\'') {
                    pos++;
                    break;
                }
                if (!cmd[pos]) {
                    *err = error_create("Missing '");
                    return 0;
                }
                pos++;
            }
        } else if (ch == '"') {
            while (1) {
                if (cmd[pos] == '"') {
                    pos++;
                    break;
                }
                if (!cmd[pos]) {
                    *err = error_create("Missing \"");
                    return 0;
                }
                if (cmd[pos++] == '\\') {
                    if (!cmd[pos]) {
                        goto unexpected_eof;
                    }
                    pos++;
                }
            }
        } else if (ch == '\\') {
            if (!cmd[pos]) {
                goto unexpected_eof;
            }
            pos++;
        } else {
            switch (ch) {
            case '*':
            case '?':
            case '[':
            case '{':
                // Reserved for glob patterns. Note that ] and } need
                // not to be reserved because those can be used alone
                // (without [ or {).
                //
                // In shell these characters are not reserved. You can run
                // "echo *" in an empty directory and it prints "*". If
                // globbing will be implemented in this program, it would
                // behave differently. Patterns should always expand to
                // "nothing" if it didn't match. E.g. "open *" in an empty
                // directory would expand to "open".
                *err = error_create (
                    "You need to escape %c (characters *?[{ are reserved)",
                    ch
                );
                return 0;
            }
        }
    }
    return pos;
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

        if (!cmd[pos]) {
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
    size_t i;
    for (i = 0; i < count; i++) {
        dst[i] = xstrdup(src[i]);
    }
    dst[i] = NULL;
    return dst;
}
