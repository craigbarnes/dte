#include <stdbool.h>
#include <stdlib.h>
#include "case.h"
#include "block-iter.h"
#include "change.h"
#include "selection.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/unicode.h"
#include "util/utf8.h"

void change_case(View *view, char mode)
{
    bool was_selecting = false;
    bool move = true;
    size_t text_len;
    if (view->selection) {
        SelectionInfo info = init_selection(view);
        view->cursor = info.si;
        text_len = info.eo - info.so;
        unselect(view);
        was_selecting = true;
        move = !info.swapped;
    } else {
        CodePoint u;
        if (!block_iter_get_char(&view->cursor, &u)) {
            return;
        }
        text_len = u_char_size(u);
    }

    String dst = string_new(text_len);
    char *src = block_iter_get_bytes(&view->cursor, text_len);
    size_t i = 0;
    switch (mode) {
    case 'l':
        while (i < text_len) {
            CodePoint u = u_to_lower(u_get_char(src, text_len, &i));
            string_append_codepoint(&dst, u);
        }
        break;
    case 'u':
        while (i < text_len) {
            CodePoint u = u_to_upper(u_get_char(src, text_len, &i));
            string_append_codepoint(&dst, u);
        }
        break;
    case 't':
        while (i < text_len) {
            CodePoint u = u_get_char(src, text_len, &i);
            u = u_is_upper(u) ? u_to_lower(u) : u_to_upper(u);
            string_append_codepoint(&dst, u);
        }
        break;
    default:
        BUG("unhandled case mode");
    }

    buffer_replace_bytes(view, text_len, dst.buffer, dst.len);
    free(src);

    if (move && dst.len > 0) {
        size_t skip = dst.len;
        if (was_selecting) {
            // Move cursor back to where it was
            u_prev_char(dst.buffer, &skip);
        }
        block_iter_skip_bytes(&view->cursor, skip);
    }

    string_free(&dst);
}
