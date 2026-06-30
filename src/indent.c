#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "indent.h"
#include "block-iter.h"
#include "buffer.h"
#include "change.h"
#include "move.h"
#include "regexp.h"
#include "selection.h"
#include "util/log.h"
#include "util/xmalloc.h"

String make_indent(const LocalOptions *options, size_t width)
{
    if (width == 0) {
        return string_new(0);
    }

    bool use_spaces = use_spaces_for_indent(options);
    size_t tw = options->tab_width;
    size_t ntabs = use_spaces ? 0 : indent_level(width, tw);
    size_t nspaces = use_spaces ? width : indent_remainder(width, tw);
    size_t nbytes = ntabs + nspaces;
    BUG_ON(nbytes == 0);

    String str = string_new(nbytes + 1); // +1 for efficiency in several callers
    memset(str.buffer, '\t', ntabs);
    memset(str.buffer + ntabs, ' ', nspaces);
    str.len = nbytes;
    return str;
}

static String make_simple_indent(const LocalOptions *options, size_t level)
{
    bool use_spaces = use_spaces_for_indent(options);
    size_t nbytes = use_spaces ? level * options->indent_width : level;
    String indent = STRING_INIT;
    string_append_memset(&indent, use_spaces ? ' ' : '\t', nbytes);
    return indent;
}

// Return true if the contents of `line` triggers an additional level
// of auto-indent on the next line
static bool line_contents_increases_indent (
    const LocalOptions *options,
    StringView line
) {
    if (!line.length) {
        return false;
    }

    static const regex_t *re1, *re2;
    if (!re1) {
        // TODO: Make these patterns configurable via a local option
        re1 = regexp_compile_or_fatal_error("\\{[\t ]*(//.*|/\\*.*\\*/[\t ]*)?$");
        re2 = regexp_compile_or_fatal_error("\\}[\t ]*(//.*|/\\*.*\\*/[\t ]*)?$");
    }

    if (options->brace_indent) {
        if (regexp_exec(re1, line.data, line.length, 0, NULL, 0)) {
            return true;
        }
        if (regexp_exec(re2, line.data, line.length, 0, NULL, 0)) {
            return false;
        }
    }

    const InternedRegexp *ir = options->indent_regex;
    if (!ir) {
        return false;
    }

    BUG_ON(ir->str[0] == '\0');
    return regexp_exec(&ir->re, line.data, line.length, 0, NULL, 0);
}

String get_indent_for_next_line(const LocalOptions *options, StringView line)
{
    size_t curr_width = get_indent_width(line, options->tab_width);
    size_t next_width = next_indent_width(curr_width, options->indent_width);
    bool increase = line_contents_increases_indent(options, line);
    return make_indent(options, increase ? next_width : curr_width);
}

IndentInfo get_indent_info(const LocalOptions *options, StringView line)
{
    const char *buf = line.data;
    const size_t len = line.length;
    const size_t tw = options->tab_width;
    const size_t iw = options->indent_width;
    const bool space_indent = use_spaces_for_indent(options);
    IndentInfo info = {.sane = true};
    size_t spaces = 0;
    size_t tabs = 0;
    size_t pos = 0;

    for (; pos < len; pos++) {
        if (buf[pos] == ' ') {
            info.width++;
            spaces++;
        } else if (buf[pos] == '\t') {
            info.width = next_indent_width(info.width, tw);
            tabs++;
        } else {
            break;
        }
        if (indent_remainder(info.width, iw) == 0 && info.sane) {
            info.sane = space_indent ? !tabs : !spaces;
        }
    }

    info.level = indent_level(info.width, iw);
    info.wsonly = (pos == len);
    info.bytes = spaces + tabs;
    return info;
}

size_t get_indent_width(StringView line, unsigned int tab_width)
{
    const char *buf = line.data;
    size_t width = 0;
    for (size_t i = 0, n = line.length; i < n; i++) {
        if (buf[i] == ' ') {
            width++;
        } else if (buf[i] == '\t') {
            width = next_indent_width(width, tab_width);
        } else {
            break;
        }
    }
    return width;
}

static ssize_t get_current_indent_bytes (
    const char *buf,
    size_t cursor_offset,
    unsigned int iw,
    unsigned int tw
) {
    size_t bytes = 0;
    size_t width = 0;

    for (size_t i = 0; i < cursor_offset; i++) {
        if (indent_remainder(width, iw) == 0) {
            bytes = 0;
            width = 0;
        }
        switch (buf[i]) {
        case '\t':
            width = next_indent_width(width, tw);
            break;
        case ' ':
            width++;
            break;
        default:
            // Cursor not at indentation
            return -1;
        }
        bytes++;
    }

    if (indent_remainder(width, iw)) {
        // Cursor at middle of indentation level
        return -1;
    }

    return (ssize_t)bytes;
}

size_t get_indent_level_bytes_left(const LocalOptions *options, const BlockIter *cursor)
{
    BlockIter bol = *cursor;
    size_t cursor_offset = block_iter_bol(&bol);
    if (cursor_offset == 0) {
        return 0; // cursor at BOL
    }

    StringView line = block_iter_get_line(&bol);
    unsigned int iw = options->indent_width;
    unsigned int tw = options->tab_width;
    ssize_t ibytes = get_current_indent_bytes(line.data, cursor_offset, iw, tw);
    return MAX(ibytes, 0);
}

size_t get_indent_level_bytes_right(const LocalOptions *options, const BlockIter *cursor)
{
    unsigned int iw = options->indent_width;
    unsigned int tw = options->tab_width;
    CurrentLineRef lr = get_current_line_and_offset(*cursor);
    ssize_t ibytes = get_current_indent_bytes(lr.line.data, lr.cursor_offset, iw, tw);
    if (ibytes < 0) {
        return 0;
    }

    size_t width = 0;
    for (size_t i = lr.cursor_offset, n = lr.line.length; i < n; i++) {
        switch (lr.line.data[i]) {
        case '\t':
            width = next_indent_width(width, tw);
            break;
        case ' ':
            width++;
            break;
        default:
            // No full indentation level at cursor position
            return 0;
        }
        if (indent_remainder(width, iw) == 0) {
            return i - lr.cursor_offset + 1;
        }
    }

    return 0;
}

static void increase_indent(View *view, size_t lines, size_t levels)
{
    BUG_ON(lines == 0);
    BUG_ON(levels == 0);
    BUG_ON(!block_iter_is_bol(&view->cursor));
    const LocalOptions *options = &view->buffer->options;
    String indent = make_simple_indent(options, levels);
    size_t i = 0;

    do {
        StringView line = block_iter_get_line(&view->cursor);
        IndentInfo info = get_indent_info(options, line);
        if (info.wsonly) {
            if (info.bytes) {
                // Remove indentation
                buffer_delete_bytes(view, info.bytes);
            }
        } else if (info.sane) {
            // Insert whitespace
            buffer_insert_bytes(view, indent.buffer, indent.len);
        } else {
            // Replace whole indentation with sane one
            String rep = make_simple_indent(options, info.level + levels);
            buffer_replace_bytes(view, info.bytes, rep.buffer, rep.len);
            string_free(&rep);
        }
    } while (++i < lines && block_iter_eat_line(&view->cursor));

    WARN_ON(i != lines);
    string_free(&indent);
}

static void decrease_indent(View *view, size_t lines, size_t levels)
{
    BUG_ON(lines == 0);
    BUG_ON(levels == 0);
    BUG_ON(!block_iter_is_bol(&view->cursor));
    const LocalOptions *options = &view->buffer->options;
    const size_t indent_width = options->indent_width;
    const bool space_indent = use_spaces_for_indent(options);
    size_t i = 0;

    do {
        StringView line = block_iter_get_line(&view->cursor);
        IndentInfo info = get_indent_info(options, line);
        if (info.wsonly) {
            if (info.bytes) {
                // Remove indentation
                buffer_delete_bytes(view, info.bytes);
            }
        } else if (info.level && info.sane) {
            size_t n = MIN(levels, info.level);
            if (space_indent) {
                n *= indent_width;
            }
            buffer_delete_bytes(view, n);
        } else if (info.bytes) {
            // Replace whole indentation with sane one
            if (info.level > levels) {
                String indent = make_simple_indent(options, info.level - levels);
                buffer_replace_bytes(view, info.bytes, indent.buffer, indent.len);
                string_free(&indent);
            } else {
                buffer_delete_bytes(view, info.bytes);
            }
        }
    } while (++i < lines && block_iter_eat_line(&view->cursor));

    WARN_ON(i != lines);
}

static void do_indent_lines(View *view, size_t lines, int levels)
{
    begin_change_chain();
    block_iter_bol(&view->cursor);
    if (levels > 0) {
        increase_indent(view, lines, levels);
    } else {
        decrease_indent(view, lines, -levels);
    }
    end_change_chain(view);
}

void indent_lines(View *view, int levels)
{
    if (unlikely(levels == 0)) {
        return;
    }

    int width = view->buffer->options.indent_width;
    BUG_ON(width < 1 || width > INDENT_WIDTH_MAX);
    long x = view_get_preferred_x(view) + (levels * width);
    x = MAX(x, 0);

    if (view->selection == SELECT_NONE) {
        do_indent_lines(view, 1, levels);
        goto out;
    }

    view->selection = SELECT_LINES;
    SelectionInfo info = init_selection(view);
    view->cursor = info.si;
    size_t lines = get_nr_selected_lines(&info);
    if (unlikely(lines == 0)) {
        return;
    }

    do_indent_lines(view, lines, levels);
    if (info.swapped) {
        // Cursor should be at beginning of selection
        block_iter_bol(&view->cursor);
        view->sel_so = block_iter_get_offset(&view->cursor);
        while (--lines) {
            block_iter_prev_line(&view->cursor);
        }
    } else {
        BlockIter save = view->cursor;
        while (--lines) {
            block_iter_prev_line(&view->cursor);
        }
        view->sel_so = block_iter_get_offset(&view->cursor);
        view->cursor = save;
    }

out:
    move_to_preferred_x(view, x);
}
