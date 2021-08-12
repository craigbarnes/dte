#include <sys/types.h>
#include "indent.h"
#include "buffer.h"
#include "regexp.h"
#include "util/xmalloc.h"
#include "view.h"

char *make_indent(size_t width)
{
    if (width == 0) {
        return NULL;
    }

    char *str;
    if (use_spaces_for_indent(buffer)) {
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
    static regex_t re1, re2;
    static bool compiled;
    if (!compiled) {
        // TODO: Make these patterns configurable via a local option
        static const char pat1[] = "\\{[\t ]*(//.*|/\\*.*\\*/[\t ]*)?$";
        static const char pat2[] = "\\}[\t ]*(//.*|/\\*.*\\*/[\t ]*)?$";
        regexp_compile_or_fatal_error(&re1, pat1, REG_NEWLINE | REG_NOSUB);
        regexp_compile_or_fatal_error(&re2, pat2, REG_NEWLINE | REG_NOSUB);
        compiled = true;
    }

    if (buffer->options.brace_indent) {
        regmatch_t m;
        if (regexp_exec(&re1, line->data, line->length, 0, &m, 0)) {
            return true;
        }
        if (regexp_exec(&re2, line->data, line->length, 0, &m, 0)) {
            return false;
        }
    }

    const char *pat = buffer->options.indent_regex;
    return pat && pat[0] && regexp_match_nosub(pat, line);
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
    const size_t tw = buffer->options.tab_width;
    const size_t iw = buffer->options.indent_width;
    const bool space_indent = use_spaces_for_indent(buffer);
    size_t spaces = 0;
    size_t tabs = 0;
    size_t pos = 0;

    *info = (IndentInfo){.sane = true};

    while (pos < len) {
        if (buf[pos] == ' ') {
            info->width++;
            spaces++;
        } else if (buf[pos] == '\t') {
            info->width = (info->width + tw) / tw * tw;
            tabs++;
        } else {
            break;
        }
        info->bytes++;
        pos++;
        if (info->width % iw == 0 && info->sane) {
            info->sane = space_indent ? !tabs : !spaces;
        }
    }

    info->level = info->width / iw;
    info->wsonly = pos == len;
}

static ssize_t get_current_indent_bytes(const char *buf, size_t cursor_offset)
{
    const size_t tw = buffer->options.tab_width;
    const size_t iw = buffer->options.indent_width;
    size_t ibytes = 0;
    size_t iwidth = 0;

    for (size_t i = 0; i < cursor_offset; i++) {
        if (iwidth % iw == 0) {
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

    if (iwidth % iw) {
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

    const size_t tw = buffer->options.tab_width;
    const size_t iw = buffer->options.indent_width;
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
        if (iwidth % iw == 0) {
            return i - cursor_offset + 1;
        }
    }
    return 0;
}

char *alloc_indent(size_t count, size_t *sizep)
{
    size_t size;
    int ch;
    if (use_spaces_for_indent(buffer)) {
        ch = ' ';
        size = count * buffer->options.indent_width;
    } else {
        ch = '\t';
        size = count;
    }

    char *indent = xmalloc(size);
    memset(indent, ch, size);
    *sizep = size;
    return indent;
}
