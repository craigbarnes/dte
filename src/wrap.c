#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "wrap.h"
#include "buffer.h"
#include "indent.h"
#include "selection.h"
#include "util/string-view.h"
#include "util/string.h"
#include "util/unicode.h"
#include "util/utf8.h"

typedef struct {
    String buf;
    String indent;
    size_t indent_width;
    size_t cur_width;
    size_t text_width;
} ParagraphFormatter;

static void add_word(ParagraphFormatter *pf, const char *word, size_t len)
{
    size_t i = 0;
    size_t word_width = 0;
    while (i < len) {
        word_width += u_char_width(u_get_char(word, len, &i));
    }

    if (pf->cur_width && pf->cur_width + 1 + word_width > pf->text_width) {
        string_append_byte(&pf->buf, '\n');
        pf->cur_width = 0;
    }

    if (pf->cur_width == 0) {
        string_append_string(&pf->buf, &pf->indent);
        pf->cur_width = pf->indent_width;
    } else {
        string_append_byte(&pf->buf, ' ');
        pf->cur_width++;
    }

    string_append_buf(&pf->buf, word, len);
    pf->cur_width += word_width;
}

static bool is_long_comment_delim(StringView sv)
{
    // TODO: make this configurable
    return strview_equal_cstring(sv, "/*") || strview_equal_cstring(sv, "*/");
}

static bool is_paragraph_separator(StringView line)
{
    strview_trim(&line);
    return (line.length == 0) || is_long_comment_delim(line);
}

static bool in_paragraph (
    StringView line,
    size_t para_indent_width,
    unsigned int tab_width
) {
    size_t w = get_indent_width(line, tab_width);
    return (w == para_indent_width) && !is_paragraph_separator(line);
}

static size_t paragraph_size(View *view)
{
    BlockIter bi = view->cursor;
    block_iter_bol(&bi);
    StringView line = block_iter_get_line(&bi);
    if (is_paragraph_separator(line)) {
        // Not in paragraph
        return 0;
    }

    unsigned int tab_width = view->buffer->options.tab_width;
    size_t para_indent_width = get_indent_width(line, tab_width);

    // Go to beginning of paragraph
    while (block_iter_prev_line(&bi)) {
        line = block_iter_get_line(&bi);
        if (!in_paragraph(line, para_indent_width, tab_width)) {
            block_iter_eat_line(&bi);
            break;
        }
    }
    view->cursor = bi;

    // Get size of paragraph
    size_t size = 0;
    do {
        size_t bytes = block_iter_eat_line(&bi);
        if (!bytes) {
            break;
        }
        size += bytes;
        line = block_iter_get_line(&bi);
    } while (in_paragraph(line, para_indent_width, tab_width));
    return size;
}

void wrap_paragraph(View *view, size_t text_width)
{
    size_t len;
    if (view->selection) {
        view->selection = SELECT_LINES;
        len = prepare_selection(view);
    } else {
        len = paragraph_size(view);
    }
    if (!len) {
        return;
    }

    const LocalOptions *options = &view->buffer->options;
    char *sel = block_iter_get_bytes(&view->cursor, len);
    StringView sv = string_view(sel, len);
    size_t indent_width = get_indent_width(sv, options->tab_width);

    ParagraphFormatter pf = {
        .buf = STRING_INIT, // TODO: Pre-allocate (based on len), to minimize reallocs
        .indent = make_indent(options, indent_width),
        .indent_width = indent_width,
        .cur_width = 0,
        .text_width = text_width
    };

    for (size_t i = 0; true; ) {
        while (i < len) {
            size_t tmp = i;
            if (!u_is_breakable_whitespace(u_get_char(sel, len, &tmp))) {
                break;
            }
            i = tmp;
        }
        if (i == len) {
            break;
        }

        size_t start = i;
        while (i < len) {
            size_t tmp = i;
            if (u_is_breakable_whitespace(u_get_char(sel, len, &tmp))) {
                break;
            }
            i = tmp;
        }

        add_word(&pf, sel + start, i - start);
    }

    if (pf.buf.len) {
        string_append_byte(&pf.buf, '\n');
    }
    buffer_replace_bytes(view, len, pf.buf.buffer, pf.buf.len);
    if (pf.buf.len) {
        block_iter_skip_bytes(&view->cursor, pf.buf.len - 1);
    }

    string_free(&pf.buf);
    string_free(&pf.indent);
    free(sel);
    unselect(view);
}
