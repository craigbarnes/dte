#include <sys/types.h>
#include "indent.h"
#include "buffer.h"
#include "regexp.h"
#include "util/str-util.h"
#include "util/xmalloc.h"
#include "view.h"

char *make_indent(size_t width)
{
    if (width == 0) {
        return NULL;
    }

    char *str;
    if (use_spaces_for_indent()) {
        str = xmalloc(width + 1);
        memset(str, ' ', width);
        str[width] = '\0';
    } else {
        size_t tw = buffer->options.tab_width;
        size_t nt = width / tw;
        size_t ns = width % tw;
        str = xmalloc(nt + ns + 1);
        memset(str, '\t', nt);
        memset(str + nt, ' ', ns);
        str[nt + ns] = '\0';
    }
    return str;
}

static bool indent_inc(const StringView *line)
{
    const char *re1 = "\\{[\t ]*(//.*|/\\*.*\\*/[\t ]*)?$";
    const char *re2 = "\\}[\t ]*(//.*|/\\*.*\\*/[\t ]*)?$";

    if (buffer->options.brace_indent) {
        if (regexp_match_nosub(re1, line)) {
            return true;
        }
        if (regexp_match_nosub(re2, line)) {
            return false;
        }
    }

    re1 = buffer->options.indent_regex;
    return re1 && *re1 && regexp_match_nosub(re1, line);
}

char *get_indent_for_next_line(const StringView *line)
{
    IndentInfo info;
    get_indent_info(line, &info);
    if (indent_inc(line)) {
        size_t w = buffer->options.indent_width;
        info.width = (info.width + w) / w * w;
    }
    return make_indent(info.width);
}

void get_indent_info(const StringView *line, IndentInfo *info)
{
    const char *buf = line->data;
    const size_t len = line->length;
    size_t spaces = 0;
    size_t tabs = 0;
    size_t pos = 0;

    MEMZERO(info);
    info->sane = true;

    while (pos < len) {
        if (buf[pos] == ' ') {
            info->width++;
            spaces++;
        } else if (buf[pos] == '\t') {
            size_t tw = buffer->options.tab_width;
            info->width = (info->width + tw) / tw * tw;
            tabs++;
        } else {
            break;
        }
        info->bytes++;
        pos++;
        if (info->width % buffer->options.indent_width == 0 && info->sane) {
            info->sane = use_spaces_for_indent() ? !tabs : !spaces;
        }
    }

    info->level = info->width / buffer->options.indent_width;
    info->wsonly = pos == len;
}

bool use_spaces_for_indent(void)
{
    return
        buffer->options.expand_tab == true
        || buffer->options.indent_width != buffer->options.tab_width;
}

static ssize_t get_current_indent_bytes(const char *buf, size_t cursor_offset)
{
    size_t tw = buffer->options.tab_width;
    size_t ibytes = 0;
    size_t iwidth = 0;

    for (size_t i = 0; i < cursor_offset; i++) {
        if (iwidth % buffer->options.indent_width == 0) {
            ibytes = 0;
            iwidth = 0;
        }
        switch (buf[i]) {
        case '\t':
            iwidth = (iwidth + tw) / tw * tw;
            break;
        case ' ':
            iwidth++;
            break;
        default:
            // Cursor not at indentation
            return -1;
        }
        ibytes++;
    }

    if (iwidth % buffer->options.indent_width) {
        // Cursor at middle of indentation level
        return -1;
    }
    return (ssize_t)ibytes;
}

size_t get_indent_level_bytes_left(void)
{
    StringView line;
    size_t cursor_offset = fetch_this_line(&view->cursor, &line);
    if (!cursor_offset) {
        return 0;
    }
    ssize_t ibytes = get_current_indent_bytes(line.data, cursor_offset);
    return (ibytes < 0) ? 0 : (size_t)ibytes;
}

size_t get_indent_level_bytes_right(void)
{
    StringView line;
    size_t cursor_offset = fetch_this_line(&view->cursor, &line);
    ssize_t ibytes = get_current_indent_bytes(line.data, cursor_offset);
    if (ibytes < 0) {
        return 0;
    }

    size_t tw = buffer->options.tab_width;
    size_t iwidth = 0;
    for (size_t i = cursor_offset, n = line.length; i < n; i++) {
        switch (line.data[i]) {
        case '\t':
            iwidth = (iwidth + tw) / tw * tw;
            break;
        case ' ':
            iwidth++;
            break;
        default:
            // No full indentation level at cursor position
            return 0;
        }
        if (iwidth % buffer->options.indent_width == 0) {
            return i - cursor_offset + 1;
        }
    }
    return 0;
}

char *alloc_indent(size_t count, size_t *sizep)
{
    char *indent;
    size_t size;
    if (use_spaces_for_indent()) {
        size = buffer->options.indent_width * count;
        indent = xmalloc(size);
        memset(indent, ' ', size);
    } else {
        size = count;
        indent = xmalloc(size);
        memset(indent, '\t', size);
    }
    *sizep = size;
    return indent;
}
