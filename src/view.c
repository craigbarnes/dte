#include "view.h"
#include "buffer.h"
#include "indent.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/str-util.h"
#include "util/utf8.h"
#include "window.h"

void view_update_cursor_y(View *view)
{
    Buffer *buffer = view->buffer;
    Block *blk;
    size_t nl = 0;
    block_for_each(blk, &buffer->blocks) {
        if (blk == view->cursor.blk) {
            nl += count_nl(blk->data, view->cursor.offset);
            view->cy = nl;
            return;
        }
        nl += blk->nl;
    }
    BUG("unreachable");
}

void view_update_cursor_x(View *view)
{
    StringView line;
    const unsigned int tw = view->buffer->options.tab_width;
    const size_t cx = fetch_this_line(&view->cursor, &line);
    long cx_char = 0;
    long w = 0;

    for (size_t idx = 0; idx < cx; cx_char++) {
        CodePoint u = line.data[idx++];
        if (likely(u < 0x80)) {
            if (likely(!ascii_iscntrl(u))) {
                w++;
            } else if (u == '\t') {
                w = next_indent_width(w, tw);
            } else {
                w += 2;
            }
        } else {
            idx--;
            u = u_get_nonascii(line.data, line.length, &idx);
            w += u_char_width(u);
        }
    }

    view->cx = cx;
    view->cx_char = cx_char;
    view->cx_display = w;
}

static bool view_is_cursor_visible(const View *v)
{
    return v->cy < v->vy || v->cy > v->vy + v->window->edit_h - 1;
}

static void view_center_to_cursor(View *v)
{
    size_t lines = v->buffer->nl;
    Window *window = v->window;
    unsigned int hh = window->edit_h / 2;

    if (window->edit_h >= lines || v->cy < hh) {
        v->vy = 0;
        return;
    }

    v->vy = v->cy - hh;
    if (v->vy + window->edit_h > lines) {
        // -1 makes one ~ line visible so that you know where the EOF is
        v->vy -= v->vy + window->edit_h - lines - 1;
    }
}

static void view_update_vx(View *v)
{
    Window *window = v->window;
    unsigned int c = 8;

    if (v->cx_display - v->vx >= window->edit_w) {
        v->vx = (v->cx_display - window->edit_w + c) / c * c;
    }
    if (v->cx_display < v->vx) {
        v->vx = v->cx_display / c * c;
    }
}

static void view_update_vy(View *v, unsigned int scroll_margin)
{
    Window *window = v->window;
    int margin = window_get_scroll_margin(window, scroll_margin);
    long max_y = v->vy + window->edit_h - 1 - margin;

    if (v->cy < v->vy + margin) {
        v->vy = MAX(v->cy - margin, 0);
    } else if (v->cy > max_y) {
        v->vy += v->cy - max_y;
        max_y = v->buffer->nl - window->edit_h + 1;
        if (v->vy > max_y && max_y >= 0) {
            v->vy = max_y;
        }
    }
}

void view_update(View *v, unsigned int scroll_margin)
{
    view_update_vx(v);
    if (v->force_center || (v->center_on_scroll && view_is_cursor_visible(v))) {
        view_center_to_cursor(v);
    } else {
        view_update_vy(v, scroll_margin);
    }
    v->force_center = false;
    v->center_on_scroll = false;
}

long view_get_preferred_x(View *v)
{
    if (v->preferred_x < 0) {
        view_update_cursor_x(v);
        v->preferred_x = v->cx_display;
    }
    return v->preferred_x;
}

bool view_can_close(const View *view)
{
    const Buffer *buffer = view->buffer;
    return !buffer_modified(buffer) || buffer->views.count > 1;
}

StringView view_do_get_word_under_cursor(const View *view, size_t *offset_in_line)
{
    StringView line;
    size_t si = fetch_this_line(&view->cursor, &line);
    while (si < line.length) {
        size_t i = si;
        if (u_is_word_char(u_get_char(line.data, line.length, &i))) {
            break;
        }
        si = i;
    }

    if (si == line.length) {
        *offset_in_line = 0;
        return string_view(NULL, 0);
    }

    size_t ei = si;
    while (si > 0) {
        size_t i = si;
        if (!u_is_word_char(u_prev_char(line.data, &i))) {
            break;
        }
        si = i;
    }

    while (ei < line.length) {
        size_t i = ei;
        if (!u_is_word_char(u_get_char(line.data, line.length, &i))) {
            break;
        }
        ei = i;
    }

    *offset_in_line = si;
    return string_view(line.data + si, ei - si);
}

StringView view_get_word_under_cursor(const View *view)
{
    size_t offset_in_line;
    return view_do_get_word_under_cursor(view, &offset_in_line);
}
