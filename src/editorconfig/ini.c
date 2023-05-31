#include "ini.h"
#include "util/debug.h"
#include "util/str-util.h"

bool ini_parse(IniParser *ctx)
{
    const char *input = ctx->input;
    const size_t len = ctx->input_len;
    size_t pos = ctx->pos;

    while (pos < len) {
        StringView line = buf_slice_next_line(input, &pos, len);
        strview_trim_left(&line);
        if (line.length < 2 || line.data[0] == '#' || line.data[0] == ';') {
            continue;
        }

        strview_trim_right(&line);
        BUG_ON(line.length == 0);
        if (line.data[0] == '[') {
            if (strview_has_suffix(&line, "]")) {
                ctx->section = string_view(line.data + 1, line.length - 2);
                ctx->name_count = 0;
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

        ctx->name = name,
        ctx->value = value,
        ctx->name_count++;
        ctx->pos = pos;
        return true;
    }

    return false;
}
