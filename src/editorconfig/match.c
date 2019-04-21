#include <regex.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "match.h"
#include "../debug.h"
#include "../util/string.h"

static bool is_regex_special_char(char ch)
{
    switch (ch) {
    case '(':
    case ')':
    case '*':
    case '+':
    case '.':
    case '?':
    case '[':
    case '\\':
    case '{':
    case '|':
        return true;
    }
    return false;
}

static size_t get_last_paired_brace_index(const char *str, size_t len)
{
    size_t last_paired_index = 0;
    size_t open_braces = 0;
    for (size_t i = 0; i < len; i++) {
        const char ch = str[i];
        switch (ch) {
        case '\\':
            i++;
            break;
        case '{':
            open_braces++;
            break;
        case '}':
            if (open_braces != 0) {
                last_paired_index = i;
                open_braces--;
            }
            break;
        }
    }
    return last_paired_index;
}

static size_t handle_bracket_expression(const char *pat, size_t len, String *buf)
{
    BUG_ON(len == 0);
    if (len == 1) {
        string_add_literal(buf, "\\[");
        return 0;
    }

    // Skip past opening bracket
    pat++;

    bool closed = false;
    size_t i = 0;
    while (i < len) {
        const char ch = pat[i++];
        if (ch == ']') {
            closed = true;
            break;
        }
    }

    if (!closed) {
        string_add_literal(buf, "\\[");
        return 0;
    }

    // TODO: interpret characters according to editorconfig instead
    // of just copying the bracket expression to be interpretted as
    // regex

    string_add_byte(buf, '[');
    if (pat[0] == '!') {
        string_add_byte(buf, '^');
        string_add_buf(buf, pat + 1, i - 1);
    } else {
        string_add_buf(buf, pat, i);
    }
    return i;
}

bool ec_pattern_match(const char *pattern, const char *path)
{
    const size_t pattern_len = strlen(pattern);
    String buf = STRING_INIT;
    size_t brace_level = 0;
    size_t last_paired_brace_index = get_last_paired_brace_index(pattern, pattern_len);

    for (size_t i = 0; i < pattern_len; i++) {
        char ch = pattern[i];
        switch (ch) {
        case '\\':
            if (i + 1 < pattern_len) {
                ch = pattern[++i];
                if (is_regex_special_char(ch)) {
                    string_add_byte(&buf, '\\');
                }
                string_add_byte(&buf, ch);
            } else {
                string_add_literal(&buf, "\\\\");
            }
            break;
        case '?':
            string_add_byte(&buf, '.');
            break;
        case '*':
            if (i + 1 < pattern_len && pattern[i + 1] == '*') {
                string_add_literal(&buf, ".*");
                i++;
            } else {
                string_add_literal(&buf, "[^/]*");
            }
            break;
        case '[':
            // The entire bracket expression is handled in a separate
            // loop because the POSIX regex escaping rules are different
            // in that context.
            i += handle_bracket_expression(pattern + i, pattern_len - i, &buf);
            break;
        case '{':
            if (i >= last_paired_brace_index) {
                string_add_literal(&buf, "\\{");
                break;
            }
            brace_level++;
            string_add_byte(&buf, '(');
            break;
        case '}':
            if (i > last_paired_brace_index || brace_level == 0) {
                goto add_byte;
            }
            brace_level--;
            string_add_byte(&buf, ')');
            break;
        case ',':
            if (i >= last_paired_brace_index || brace_level == 0) {
                goto add_byte;
            }
            // TODO: don't emit empty alternates in regex (it's "undefined").
            // Instead, record that the set contains a null pattern and
            // emit ")?" instead of ")" as the closing delimiter.
            string_add_byte(&buf, '|');
            break;
        case '/':
            if (i + 3 < pattern_len && memcmp(pattern + i, "/**/", 4) == 0) {
                string_add_literal(&buf, "(/|/.*/)");
                i += 3;
                break;
            }
            goto add_byte;
        case '.':
        case '(':
        case ')':
        case '|':
        case '+':
            string_add_byte(&buf, '\\');
            // Fallthrough
        default:
        add_byte:
            string_add_byte(&buf, ch);
        }
    }

    string_add_byte(&buf, '$');
    char *regex_pattern = string_steal_cstring(&buf);

    regex_t re;
    bool compiled = !regcomp(&re, regex_pattern, REG_EXTENDED | REG_NOSUB);
    free(regex_pattern);
    if (!compiled) {
        return false;
    }

    int res = regexec(&re, path, 0, NULL, 0);
    regfree(&re);
    return res == 0;
}
