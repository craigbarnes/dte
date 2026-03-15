#include <regex.h>
#include <stdlib.h>
#include "match.h"
#include "regexp.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/xstring.h"

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
            if (open_braces >= 32) {
                // If nesting goes too deep, just return 0 and let
                // ec_pattern_match() escape all braces
                return 0;
            }
            break;
        case '}':
            if (open_braces != 0) {
                last_paired_index = i;
                open_braces--;
            }
            break;
        }
    }

    // If there are unclosed braces, just return 0
    return open_braces ? 0 : last_paired_index;
}

static size_t handle_bracket_expression(const char *pat, size_t len, String *buf)
{
    BUG_ON(len == 0);
    if (len == 1) {
        string_append_literal(buf, "\\[");
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
        string_append_literal(buf, "\\[");
        return 0;
    }

    // TODO: interpret characters according to editorconfig instead
    // of just copying the bracket expression to be interpreted as
    // regex

    char *s = string_reserve_space(buf, i + 1);
    s[0] = '[';
    memcpy(s + 1, pat, i);
    s[1] = (s[1] == '!') ? '^' : s[1]; // Replace any leading ! with ^
    buf->len += i + 1;
    return i;
}

// Skips past empty alternates in brace groups and returns the number
// of bytes (commas) skipped
static size_t skip_empty_alternates(const char *str, size_t len)
{
    size_t i = 1;
    while (i < len && str[i] == ',') {
        i++;
    }
    return i - 1;
}

static void section_to_regex(String *buf, StringView section)
{
    const char *pattern = section.data;
    size_t pattern_len = section.length;
    size_t brace_level = 0;
    size_t last_paired_brace_index = get_last_paired_brace_index(pattern, pattern_len);
    bool brace_group_has_empty_alternate[32] = {false};

    for (size_t i = 0; i < pattern_len; i++) {
        char ch = pattern[i];
        switch (ch) {
        case '\\':
            if (i + 1 < pattern_len) {
                ch = pattern[++i];
                if (is_regex_special_char(ch)) {
                    string_append_byte(buf, '\\');
                }
                string_append_byte(buf, ch);
            } else {
                string_append_literal(buf, "\\\\");
            }
            break;
        case '?':
            string_append_literal(buf, "[^/]");
            break;
        case '*':
            if (i + 1 < pattern_len && pattern[i + 1] == '*') {
                string_append_literal(buf, ".*");
                i++;
            } else {
                string_append_literal(buf, "[^/]*");
            }
            break;
        case '[':
            // The entire bracket expression is handled in a separate
            // loop because the POSIX regex escaping rules are different
            // in that context
            i += handle_bracket_expression(pattern + i, pattern_len - i, buf);
            break;
        case '{': {
            if (i >= last_paired_brace_index) {
                string_append_literal(buf, "\\{");
                break;
            }
            brace_level++;
            size_t skip = skip_empty_alternates(pattern + i, pattern_len - i);
            if (skip > 0) {
                i += skip;
                brace_group_has_empty_alternate[brace_level] = true;
            }
            if (i + 1 < pattern_len && pattern[i + 1] == '}') {
                // If brace group contains only empty alternates, emit nothing
                brace_group_has_empty_alternate[brace_level] = false;
                i++;
                brace_level--;
            } else {
                string_append_byte(buf, '(');
            }
            break;
        }
        case '}':
            if (i > last_paired_brace_index || brace_level == 0) {
                goto append_byte;
            }
            string_append_byte(buf, ')');
            if (brace_group_has_empty_alternate[brace_level]) {
                string_append_byte(buf, '?');
            }
            brace_group_has_empty_alternate[brace_level] = false;
            brace_level--;
            break;
        case ',': {
            if (i >= last_paired_brace_index || brace_level == 0) {
                goto append_byte;
            }
            size_t skip = skip_empty_alternates(pattern + i, pattern_len - i);
            if (skip > 0) {
                i += skip;
                brace_group_has_empty_alternate[brace_level] = true;
            }
            if (i + 1 < pattern_len && pattern[i + 1] == '}') {
                brace_group_has_empty_alternate[brace_level] = true;
            } else {
                string_append_byte(buf, '|');
            }
            break;
        }
        case '/':
            if (i + 3 < pattern_len && mem_equal(pattern + i, "/**/", 4)) {
                string_append_literal(buf, "/(.*/)?");
                i += 3;
                break;
            }
            goto append_byte;
        case '.':
        case '(':
        case ')':
        case '|':
        case '+':
            string_append_byte(buf, '\\');
            // Fallthrough
        default:
        append_byte:
            string_append_byte(buf, ch);
        }
    }
}

String ec_pattern_to_regex(StringView section, StringView dir)
{
    BUG_ON(section.length == 0);
    BUG_ON(dir.length == 0);

    String s = string_new((2 * dir.length) + section.length + 16);
    string_append_byte(&s, '^');
    string_append_escaped_regex(&s, dir);

    if (!strview_memchr(section, '/')) {
        // Section contains no slashes; insert "/(.*/)?" between `dir`
        // and `section`, so as to match files in or below `dir`
        string_append_literal(&s, "/(.*/)?");
    } else if (section.data[0] != '/') {
        // Section contains slashes, but not at the start; insert '/'
        // between `dir` and `section`
        string_append_byte(&s, '/');
    }

    section_to_regex(&s, section);
    string_append_byte(&s, '$');
    return s;
}

bool ec_pattern_match(StringView section, StringView dir, const char *path)
{
    String re_str = ec_pattern_to_regex(section, dir);
    int flags = REG_EXTENDED | REG_NOSUB;
    regex_t re;
    bool compiled = !regcomp(&re, string_borrow_cstring(&re_str), flags);
    string_free(&re_str);

    if (!compiled) {
        return false;
    }

    bool match = !regexec(&re, path, 0, NULL, 0);
    regfree(&re);
    return match;
}
