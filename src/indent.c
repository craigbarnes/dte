#include "indent.h"
#include "buffer.h"
#include "view.h"
#include "regexp.h"

char *make_indent(int width)
{
    char *str;

    if (width == 0) {
        return NULL;
    }

    if (use_spaces_for_indent()) {
        str = xnew(char, width + 1);
        memset(str, ' ', width);
        str[width] = 0;
    } else {
        int tw = buffer->options.tab_width;
        int nt = width / tw;
        int ns = width % tw;

        str = xnew(char, nt + ns + 1);
        memset(str, '\t', nt);
        memset(str + nt, ' ', ns);
        str[nt + ns] = 0;
    }
    return str;
}

static bool indent_inc(const char *line, unsigned int len)
{
    const char *re1 = "\\{[\t ]*(//.*|/\\*.*\\*/[\t ]*)?$";
    const char *re2 = "\\}[\t ]*(//.*|/\\*.*\\*/[\t ]*)?$";

    if (buffer->options.brace_indent) {
        if (regexp_match_nosub(re1, line, len)) {
            return true;
        }
        if (regexp_match_nosub(re2, line, len)) {
            return false;
        }
    }

    re1 = buffer->options.indent_regex;
    return *re1 && regexp_match_nosub(re1, line, len);
}

char *get_indent_for_next_line(const char *line, unsigned int len)
{
    IndentInfo info;

    get_indent_info(line, len, &info);
    if (indent_inc(line, len)) {
        int w = buffer->options.indent_width;
        info.width = (info.width + w) / w * w;
    }
    return make_indent(info.width);
}

void get_indent_info(const char *buf, int len, IndentInfo *info)
{
    int spaces = 0;
    int tabs = 0;
    int pos = 0;

    memzero(info);
    info->sane = true;
    while (pos < len) {
        if (buf[pos] == ' ') {
            info->width++;
            spaces++;
        } else if (buf[pos] == '\t') {
            int tw = buffer->options.tab_width;
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

static int get_current_indent_bytes(const char *buf, int cursor_offset)
{
    int tw = buffer->options.tab_width;
    int ibytes = 0;
    int iwidth = 0;

    for (int i = 0; i < cursor_offset; i++) {
        char ch = buf[i];

        if (iwidth % buffer->options.indent_width == 0) {
            ibytes = 0;
            iwidth = 0;
        }

        if (ch == '\t') {
            iwidth = (iwidth + tw) / tw * tw;
        } else if (ch == ' ') {
            iwidth++;
        } else {
            // Cursor not at indentation
            return -1;
        }
        ibytes++;
    }

    if (iwidth % buffer->options.indent_width) {
        // Cursor at middle of indentation level
        return -1;
    }
    return ibytes;
}

int get_indent_level_bytes_left(void)
{
    LineRef lr;
    unsigned int cursor_offset = fetch_this_line(&view->cursor, &lr);
    int ibytes;

    if (!cursor_offset) {
        return 0;
    }

    ibytes = get_current_indent_bytes(lr.line, cursor_offset);
    if (ibytes < 0) {
        return 0;
    }
    return ibytes;
}

int get_indent_level_bytes_right(void)
{
    LineRef lr;
    unsigned int cursor_offset = fetch_this_line(&view->cursor, &lr);
    int tw = buffer->options.tab_width;
    int i, ibytes, iwidth;

    ibytes = get_current_indent_bytes(lr.line, cursor_offset);
    if (ibytes < 0) {
        return 0;
    }

    iwidth = 0;
    for (i = cursor_offset; i < lr.size; i++) {
        char ch = lr.line[i];

        if (ch == '\t') {
            iwidth = (iwidth + tw) / tw * tw;
        } else if (ch == ' ') {
            iwidth++;
        } else {
            // No full indentation level at cursor position
            return 0;
        }

        if (iwidth % buffer->options.indent_width == 0) {
            return i - cursor_offset + 1;
        }
    }
    return 0;
}

char *alloc_indent(int count, int *sizep)
{
    char *indent;
    int size;

    if (use_spaces_for_indent()) {
        size = buffer->options.indent_width * count;
        indent = xnew(char, size);
        memset(indent, ' ', size);
    } else {
        size = count;
        indent = xnew(char, size);
        memset(indent, '\t', size);
    }
    *sizep = size;
    return indent;
}
