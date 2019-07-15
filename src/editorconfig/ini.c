#include <stdlib.h>
#include <string.h>
#include "ini.h"
#include "../debug.h"
#include "../util/ascii.h"
#include "../util/readfile.h"

static char *trim_left(char *str)
{
    while (ascii_isspace(*str)) {
        str++;
    }
    return str;
}

static void strip_trailing_comments_and_whitespace(StringView *line)
{
    const char *str = line->data;
    size_t len = line->length;

    // Remove inline comments
    char prev_char = '\0';
    for (size_t i = len; i > 0; i--) {
        if (ascii_isspace(str[i]) && (prev_char == '#' || prev_char == ';')) {
            len = i;
        }
        prev_char = str[i];
    }

    // Trim trailing whitespace
    const char *ptr = str + len - 1;
    while (ptr > str && ascii_isspace(*ptr--)) {
        len--;
    }

    line->length = len;
}

UNITTEST {
    StringView tmp = STRING_VIEW(" \t  key = val   #   inline comment    ");
    string_view_trim_left(&tmp);
    strip_trailing_comments_and_whitespace(&tmp);
    BUG_ON(!string_view_equal_literal(&tmp, "key = val"));
}

int ini_parse(const char *filename, IniCallback callback, void *userdata)
{
    char *buf;
    const ssize_t ssize = read_file(filename, &buf);
    if (ssize < 0) {
        return -1;
    }

    const size_t size = ssize;
    size_t pos = 0;
    if (size >= 3 && memcmp(buf, "\xEF\xBB\xBF", 3) == 0) {
        // Skip past UTF-8 BOM
        pos += 3;
    }

    StringView section = STRING_VIEW_INIT;
    unsigned int nameidx = 0;

    while (pos < size) {
        StringView line = buf_slice_next_line(buf, &pos, size);
        string_view_trim_left(&line);

        if (line.length < 2) {
            continue;
        }

        switch (line.data[0]) {
        case ';':
        case '#':
            continue;
        case '[':
            strip_trailing_comments_and_whitespace(&line);
            if (line.length > 1 && line.data[line.length - 1] == ']') {
                section = string_view(line.data + 1, line.length - 2);
                nameidx = 0;
            }
            continue;
        }

        strip_trailing_comments_and_whitespace(&line);
        char *delim = string_view_memchr(&line, '=');
        if (delim) {
            const size_t before_delim_len = delim - line.data;
            size_t name_len = before_delim_len;
            while (name_len > 0 && ascii_isblank(line.data[name_len - 1])) {
                name_len--;
            }
            if (name_len == 0) {
                continue;
            }

            char *after_delim = delim + 1;
            char *value = trim_left(after_delim);
            size_t diff = value - after_delim;
            size_t value_len = line.length - before_delim_len - 1 - diff;

            const IniData data = {
                .section = section,
                .name = string_view(line.data, name_len),
                .value = string_view(value, value_len),
                .name_idx = nameidx++,
            };

            callback(&data, userdata);
        }
    }

    free(buf);
    return 0;
}
