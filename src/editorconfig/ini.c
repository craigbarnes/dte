#include "ini.h"
#include "util/debug.h"
#include "util/str-util.h"

/*
 * This is a "pull" style INI parser that returns true (and fills
 * ctx->name and ctx->value) for each name=value pair encountered,
 * or false when there's nothing further to parse. The current
 * section name is tracked as ctx->section and lines that aren't
 * actionable by the caller (i.e. section/comment/blank/invalid
 * lines) are skipped over without returning anything.
 *
 * Note that "inline" comments are not supported, since the
 * EditorConfig specification forbids them and that's the only
 * use case in this codebase (see commit a61b90f630cd6a32).
 */
bool ini_parse(IniParser *ctx)
{
    const char *input = ctx->input;
    const size_t len = ctx->input_len;
    size_t pos = ctx->pos;

    while (pos < len) {
        StringView line = buf_slice_next_line(input, &pos, len);
        strview_trim_left(&line);
        if (line.length < 2 || line.data[0] == '#' || line.data[0] == ';') {
            // Skip past comment/blank lines and lines that are too short
            // to be valid (shorter than "k=")
            continue;
        }

        strview_trim_right(&line);
        BUG_ON(line.length == 0);

        if (strview_remove_matching_affixes(&line, strview("["), strview("]"))) {
            // Keep track of (and skip past) section headings
            ctx->section = line;
            ctx->name_count = 0;
            continue;
        }

        size_t val_offset = 0;
        StringView name = get_delim(line.data, &val_offset, line.length, '=');
        if (val_offset >= line.length) {
            continue; // Invalid line (no delimiter)
        }

        strview_trim_right(&name);
        if (name.length == 0) {
            continue; // Invalid line (empty name)
        }

        StringView value = line;
        strview_remove_prefix(&value, val_offset);
        strview_trim_left(&value);

        ctx->name = name;
        ctx->value = value;
        ctx->name_count++;
        ctx->pos = pos;
        return true;
    }

    return false;
}
