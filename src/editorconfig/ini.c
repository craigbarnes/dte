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

static char *trim_right(char *str, size_t len)
{
    char *ptr = str + len - 1;
    while (ptr > str && ascii_isspace(*ptr--)) {
        len--;
    }
    str[len] = '\0';
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

    const char *section = "";
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
                line[line_len - 1] = '\0';
                section = line + 1;
                nameidx = 0;
            }
            continue;
        }

        strip_inline_comments(line);
        char *delim = strchr(line, '=');
        if (delim) {
            *delim = '\0';
            size_t n = delim - line;
            BUG_ON(strlen(line) != n);
            char *name = trim_right(line, n);
            char *value = trim_left(delim + 1);
            callback(userdata, section, name, value, nameidx++);
        }
    }

    free(buf);
    return 0;
}
