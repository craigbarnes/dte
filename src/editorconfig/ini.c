#include <stdlib.h>
#include <string.h>
#include "ini.h"
#include "../common.h"
#include "../debug.h"
#include "../util/ascii.h"

static char *trim_left(char *str)
{
    while (ascii_isspace(*str)) {
        str++;
    }
    return str;
}

static size_t strip_inline_comments(char *const str)
{
    size_t len = strlen(str);

    // Remove inline comments
    char prev_char = '\0';
    for (size_t i = len; i > 0; i--) {
        if (ascii_isspace(str[i]) && (prev_char == '#' || prev_char == ';')) {
            len = i;
        }
        prev_char = str[i];
    }

    // Trim trailing whitespace
    char *ptr = str + len - 1;
    while (ptr > str && ascii_isspace(*ptr--)) {
        len--;
    }

    str[len] = '\0';
    return len;
}

UNITTEST {
    char tmp[64] = " \t  key = val   #   inline comment    ";
    strip_inline_comments(tmp);
    char *trimmed = trim_left(tmp);
    DEBUG_VAR(trimmed);
    BUG_ON(!streq(trimmed, "key = val"));
    tmp[0] = '\0';
    strip_inline_comments(tmp);
    BUG_ON(!streq(tmp, ""));
}

int ini_parse(const char *filename, IniCallback callback, void *userdata)
{
    char *buf;
    ssize_t size = read_file(filename, &buf);
    if (size < 0) {
        return -1;
    }

    ssize_t pos = 0;
    if (size >= 3 && memcmp(buf, "\xEF\xBB\xBF", 3) == 0) {
        // Skip past UTF-8 BOM
        pos += 3;
    }

    StringView section = STRING_VIEW_INIT;
    unsigned int nameidx = 0;

    while (pos < size) {
        char *line = trim_left(buf_next_line(buf, &pos, size));
        size_t line_len;

        switch (line[0]) {
        case ';':
        case '#':
            continue;
        case '[':
            line_len = strip_inline_comments(line);
            if (line_len > 1 && line[line_len - 1] == ']') {
                section = string_view(line + 1, line_len - 2);
                nameidx = 0;
            }
            continue;
        }

        line_len = strip_inline_comments(line);
        char *delim = memchr(line, '=', line_len);
        if (delim) {
            const size_t before_delim_len = delim - line;
            size_t name_len = before_delim_len;
            while (name_len > 0 && ascii_isblank(line[name_len - 1])) {
                name_len--;
            }
            if (name_len == 0) {
                continue;
            }

            char *after_delim = delim + 1;
            char *value = trim_left(after_delim);
            size_t diff = value - after_delim;
            size_t value_len = line_len - before_delim_len - 1 - diff;

            const IniData data = {
                .section = section,
                .name = string_view(line, name_len),
                .value = string_view(value, value_len),
                .name_idx = nameidx++,
            };

            callback(&data, userdata);
        }
    }

    free(buf);
    return 0;
}
