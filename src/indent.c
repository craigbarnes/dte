#include <string.h>
#include <sys/types.h>
#include "indent.h"
#include "block-iter.h"
#include "buffer.h"
#include "regexp.h"
#include "util/xmalloc.h"
#include "view.h"

char *make_indent(const Buffer *buffer, size_t width)
{
    if (width == 0) {
        return NULL;
    }

    size_t ntabs, nspaces;
    if (use_spaces_for_indent(buffer)) {
        ntabs = 0;
        nspaces = width;
    } else {
        size_t tw = buffer->options.tab_width;
        ntabs = width / tw;
        nspaces = width % tw;
    }

    size_t n = ntabs + nspaces;
    char *str = xmalloc(n + 1);
    memset(str, '\t', ntabs);
    memset(str + ntabs, ' ', nspaces);
    str[n] = '\0';
    return str;
}

static bool indent_inc(const Buffer *buffer, const StringView *line)
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
    if (!pat || pat[0] == '\0') {
        return false;
    }

    regex_t re;
    int err = regcomp(&re, pat, REG_EXTENDED | REG_NOSUB | REG_NEWLINE);
    if (unlikely(err)) {
        // Note: the indent_regex pattern has already been checked by
        // validate_regex(), so the only error that should be possible
        // here is REG_ESPACE ("ran out of memory").
        if (DEBUG >= 2) {
            char msg[1024];
            regerror(err, &re, msg, sizeof(msg));
            LOG_ERROR("regcomp: %s", msg);
        }
        return false;
    }

    regmatch_t m;
    bool ret = regexp_exec(&re, line->data, line->length, 0, &m, 0);
    regfree(&re);
    return ret;
}

char *get_indent_for_next_line(const Buffer *buffer, const StringView *line)
{
    IndentInfo info;
    get_indent_info(buffer, line, &info);
    if (indent_inc(buffer, line)) {
        size_t w = buffer->options.indent_width;
        info.width = (info.width + w) / w * w;
    }
    return make_indent(buffer, info.width);
}

void get_indent_info(const Buffer *buffer, const StringView *line, IndentInfo *info)
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

static ssize_t get_current_indent_bytes(const Buffer *buffer, const char *buf, size_t cursor_offset)
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

size_t get_indent_level_bytes_left(const View *view)
{
    StringView line;
    size_t cursor_offset = fetch_this_line(&view->cursor, &line);
    if (!cursor_offset) {
        return 0;
    }
    ssize_t ibytes = get_current_indent_bytes(view->buffer, line.data, cursor_offset);
    return (ibytes < 0) ? 0 : (size_t)ibytes;
}

size_t get_indent_level_bytes_right(const View *view)
{
    StringView line;
    size_t cursor_offset = fetch_this_line(&view->cursor, &line);
    ssize_t ibytes = get_current_indent_bytes(view->buffer, line.data, cursor_offset);
    if (ibytes < 0) {
        return 0;
    }

    const size_t tw = view->buffer->options.tab_width;
    const size_t iw = view->buffer->options.indent_width;
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
