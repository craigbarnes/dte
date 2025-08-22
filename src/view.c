#include "view.h"
#include "block.h"
#include "buffer.h"
#include "indent.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/numtostr.h"
#include "util/str-util.h"
#include "util/time-util.h"
#include "util/utf8.h"
#include "window.h"

void view_update_cursor_y(View *view)
{
    const Buffer *buffer = view->buffer;
    const Block *blk;
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
    const size_t cx = get_current_line_and_offset(&view->cursor, &line);
    long cx_char = 0;
    long w = 0;

    for (size_t idx = 0; idx < cx; cx_char++) {
        unsigned char ch = line.data[idx];
        if (likely(ch < 0x80)) {
            idx++;
            if (likely(!ascii_iscntrl(ch))) {
                w++;
            } else if (ch == '\t') {
                w = next_indent_width(w, tw);
            } else {
                w += 2;
            }
        } else {
            CodePoint u = u_get_nonascii(line.data, line.length, &idx);
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

static void view_update_vy(View *v)
{
    Window *window = v->window;
    long margin = window_get_scroll_margin(window);
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

void view_update(View *v)
{
    view_update_cursor_x(v);
    view_update_cursor_y(v);
    view_update_vx(v);
    if (v->force_center || (v->center_on_scroll && view_is_cursor_visible(v))) {
        view_center_to_cursor(v);
    } else {
        view_update_vy(v);
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

// Like window_remove_view(), but searching for the index of `view` in
// `view->window->views` by pointer
size_t view_remove(View *view)
{
    Window *window = view->window;
    size_t view_idx = ptr_array_xindex(&window->views, view);
    window_remove_view_at_index(window, view_idx);
    return view_idx;
}

size_t get_bounds_for_word_under_cursor(StringView line, size_t *cursor_offset)
{
    // Move right, until over a word char (if not already)
    size_t si = *cursor_offset;
    while (si < line.length) {
        size_t i = si;
        if (u_is_word_char(u_get_char(line.data, line.length, &i))) {
            break;
        }
        si = i;
    }

    if (si == line.length) {
        // No word char between cursor and EOL; no word
        return 0;
    }

    // Move left, to start of word (if cursor is already within one)
    size_t ei = si;
    while (si > 0) {
        size_t i = si;
        if (!u_is_word_char(u_prev_char(line.data, &i))) {
            break;
        }
        si = i;
    }

    // Move right, to end of word
    while (ei < line.length) {
        size_t i = ei;
        if (!u_is_word_char(u_get_char(line.data, line.length, &i))) {
            break;
        }
        ei = i;
    }

    if (si == ei) {
        // Zero length; no word
        return 0;
    }

    // Update `cursor_offset` with start offset and return end offset
    BUG_ON(ei == 0 || si >= ei);
    *cursor_offset = si;
    return ei;
}

StringView view_get_word_under_cursor(const View *view)
{
    StringView line;
    size_t cursor_offset_in_line = get_current_line_and_offset(&view->cursor, &line);
    size_t start = cursor_offset_in_line;
    size_t end = get_bounds_for_word_under_cursor(line, &start);
    return string_view(line.data + start, end ? end - start : 0);
}

String dump_buffer(const View *view)
{
    const Buffer *buffer = view->buffer;
    uintmax_t counts[2];
    char sizestr[FILESIZE_STR_MAX];
    buffer_count_blocks_and_bytes(buffer, counts);
    BUG_ON(counts[0] < 1);
    BUG_ON(!buffer->setup);
    String buf = string_new(1024 + (DEBUG ? 24 * counts[0] : 0));

    string_sprintf (
        &buf,
        "%s %s\n%s %lu\n%s %s\n%s %s\n%s %ju\n%s %zu\n%s %s\n",
        "     Name:", buffer_filename(buffer),
        "       ID:", buffer->id,
        " Encoding:", buffer->encoding,
        " Filetype:", buffer->options.filetype,
        "   Blocks:", counts[0],
        "    Lines:", buffer->nl,
        "     Size:", filesize_to_str(counts[1], sizestr)

    );

    if (
        buffer->stdout_buffer || buffer->temporary || buffer->readonly
        || buffer->locked || buffer->crlf_newlines || buffer->bom
    ) {
        string_sprintf (
            &buf,
            "    Flags:%s%s%s%s%s%s\n",
            buffer->stdout_buffer ? " STDOUT" : "",
            buffer->temporary ? " TMP" : "",
            buffer->readonly ? " RO" : "",
            buffer->locked ? " LOCKED" : "",
            buffer->crlf_newlines ? " CRLF" : "",
            buffer->bom ? " BOM" : ""
        );
    }

    if (buffer->views.count > 1) {
        string_sprintf(&buf, "    Views: %zu\n", buffer->views.count);
    }

    if (buffer->abs_filename) {
        const FileInfo *file = &buffer->file;
        const struct timespec *mtime = &file->mtime;
        unsigned int perms = file->mode & 07777;
        char tstr[TIME_STR_BUFSIZE];
        char modestr[12];
        string_sprintf (
            &buf,
            "\nLast stat:\n----------\n\n"
            "%s %s\n%s %s\n%s -%s (%04o)\n%s %jd\n%s %jd\n%s %s\n%s %jd\n%s %ju\n",
            "     Path:", buffer->abs_filename,
            " Modified:", timespec_to_str(mtime, tstr) ? tstr : "-",
            "     Mode:", file_permissions_to_str(file->mode, modestr), perms,
            "     User:", (intmax_t)file->uid,
            "    Group:", (intmax_t)file->gid,
            "     Size:", filesize_to_str(file->size, sizestr),
            "   Device:", (intmax_t)file->dev,
            "    Inode:", (uintmax_t)file->ino
        );
    }

    if (DEBUG >= 1) {
        const BlockIter *cursor = &view->cursor;
        string_append_cstring(&buf, "\nBlocks:\n-------\n\n");
        size_t i = 1;
        const Block *b;
        block_for_each(b, &buffer->blocks) {
            string_sprintf(&buf, "%4zu: %zu/%zu nl=%zu", i++, b->size, b->alloc, b->nl);
            if (b == cursor->blk) {
                string_sprintf(&buf, " (cursor; offset=%zu)", cursor->offset);
            }
            string_append_byte(&buf, '\n');
        }
    }

    return buf;
}
