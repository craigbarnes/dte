#include <stdlib.h>
#include "ini.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/macros.h"
#include "util/readfile.h"
#include "util/str-util.h"

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
    BUG_ON(strview_trim_left(&tmp) != 4);
    strip_trailing_comments_and_whitespace(&tmp);
    BUG_ON(!strview_equal_cstring(&tmp, "key = val"));
}

void ini_parse(const char *buf, size_t size, IniCallback callback, void *userdata)
{
    size_t pos = 0;
    if (size >= 3 && mem_equal(buf, "\xEF\xBB\xBF", 3)) {
        // Skip past UTF-8 BOM
        pos += 3;
    }

    StringView section = STRING_VIEW_INIT;
    unsigned int nameidx = 0;

    while (pos < size) {
        StringView line = buf_slice_next_line(buf, &pos, size);
        strview_trim_left(&line);
        if (line.length < 2 || line.data[0] == '#' || line.data[0] == ';') {
            continue;
        }

        strip_trailing_comments_and_whitespace(&line);
        BUG_ON(line.length == 0);
        if (line.data[0] == '[') {
            if (strview_has_suffix(&line, "]")) {
                section = string_view(line.data + 1, line.length - 2);
                nameidx = 0;
            }
            continue;
        }

        size_t val_offset = 0;
        StringView name = get_delim(line.data, &val_offset, line.length, '=');
        if (val_offset >= line.length) {
            continue;
        }

        strview_trim_right(&name);
        if (name.length == 0) {
            continue;
        }

        StringView value = line;
        strview_remove_prefix(&value, val_offset);
        strview_trim_left(&value);

        const IniData data = {
            .section = section,
            .name = name,
            .value = value,
            .name_idx = nameidx++,
        };

        callback(&data, userdata);
    }
}
