#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"
#include "buffer.h"
#include "editor.h"
#include "error.h"
#include "misc.h"
#include "move.h"
#include "search.h"
#include "util/debug.h"
#include "util/numtostr.h"
#include "util/path.h"
#include "util/xmalloc.h"
#include "window.h"

void file_location_free(FileLocation *loc)
{
    free(loc->filename);
    free(loc->pattern);
    free(loc);
}

FileLocation *get_current_file_location(const View *view)
{
    const char *filename = view->buffer->abs_filename;
    FileLocation *loc = xmalloc(sizeof(*loc));
    *loc = (FileLocation) {
        .filename = filename ? xstrdup(filename) : NULL,
        .buffer_id = view->buffer->id,
        .line = view->cy + 1,
        .column = view->cx_char + 1
    };
    return loc;
}

static bool file_location_go(const FileLocation *loc)
{
    Window *window = editor.window;
    View *view = window_open_buffer(window, loc->filename, true, NULL);
    if (!view) {
        // Failed to open file. Error message should be visible.
        return false;
    }

    if (window->view != view) {
        set_view(&editor, view);
        // Force centering view to the cursor because file changed
        view->force_center = true;
    }

    bool ok = true;
    if (loc->pattern) {
        bool err = false;
        search_tag(view, loc->pattern, &err);
        ok = !err;
    } else if (loc->line > 0) {
        move_to_line(view, loc->line);
        if (loc->column > 0) {
            move_to_column(view, loc->column);
        }
    }
    if (ok) {
        unselect(view);
    }
    return ok;
}

static bool file_location_return(const FileLocation *loc)
{
    Window *window = editor.window;
    Buffer *buffer = find_buffer_by_id(loc->buffer_id);
    View *view;

    if (buffer) {
        view = window_get_view(window, buffer);
    } else {
        if (!loc->filename) {
            // Can't restore closed buffer that had no filename.
            // Try again.
            return false;
        }
        view = window_open_buffer(window, loc->filename, true, NULL);
    }

    if (!view) {
        // Open failed. Don't try again.
        return true;
    }

    set_view(&editor, view);
    unselect(view);
    move_to_line(view, loc->line);
    move_to_column(view, loc->column);
    return true;
}

void push_file_location(PointerArray *locations, FileLocation *loc)
{
    const size_t max_entries = 256;
    if (locations->count == max_entries) {
        file_location_free(ptr_array_remove_idx(locations, 0));
    }
    BUG_ON(locations->count >= max_entries);
    ptr_array_append(locations, loc);
}

void pop_file_location(PointerArray *locations)
{
    void **ptrs = locations->ptrs;
    size_t count = locations->count;
    bool go = true;
    while (count > 0 && go) {
        FileLocation *loc = ptrs[--count];
        go = !file_location_return(loc);
        file_location_free(loc);
    }
    locations->count = count;
}

static void free_message(Message *m)
{
    free(m->msg);
    if (m->loc) {
        file_location_free(m->loc);
    }
    free(m);
}

Message *new_message(const char *msg, size_t len)
{
    Message *m = xnew0(Message, 1);
    m->msg = xstrcut(msg, len);
    return m;
}

void add_message(MessageArray *msgs, Message *m)
{
    ptr_array_append(&msgs->array, m);
}

void activate_current_message(const MessageArray *msgs)
{
    size_t msg_pos = msgs->pos;
    if (msg_pos == msgs->array.count) {
        return;
    }
    const Message *m = msgs->array.ptrs[msg_pos];
    if (m->loc && m->loc->filename) {
        if (!file_location_go(m->loc)) {
            // Error message is visible
            return;
        }
    }
    if (msgs->array.count == 1) {
        info_msg("%s", m->msg);
    } else {
        info_msg("[%zu/%zu] %s", msg_pos + 1, msgs->array.count, m->msg);
    }
}

void activate_message(MessageArray *msgs, size_t idx)
{
    const size_t count = msgs->array.count;
    if (count == 0) {
        return;
    }
    msgs->pos = (idx < count) ? idx : count - 1;
    activate_current_message(msgs);
}

void activate_next_message(MessageArray *msgs)
{
    if (msgs->pos + 1 < msgs->array.count) {
        msgs->pos++;
    }
    activate_current_message(msgs);
}

void activate_prev_message(MessageArray *msgs)
{
    if (msgs->pos > 0) {
        msgs->pos--;
    }
    activate_current_message(msgs);
}

void activate_current_message_save(const MessageArray *arr, PointerArray *file_locations, const View *view)
{
    const BlockIter save = view->cursor;
    FileLocation *loc = get_current_file_location(view);
    activate_current_message(arr);

    // Save position if file changed or cursor moved
    view = editor.view;
    if (view->cursor.blk != save.blk || view->cursor.offset != save.offset) {
        push_file_location(file_locations, loc);
    } else {
        file_location_free(loc);
    }
}

void clear_messages(MessageArray *msgs)
{
    ptr_array_free_cb(&msgs->array, FREE_FUNC(free_message));
    msgs->pos = 0;
}

String dump_messages(const MessageArray *messages)
{
    String buf = string_new(4096);
    char cwd[8192];
    if (unlikely(!getcwd(cwd, sizeof cwd))) {
        return buf;
    }

    for (size_t i = 0, n = messages->array.count; i < n; i++) {
        const Message *m = messages->array.ptrs[i];
        string_sprintf(&buf, "%zu: ", i + 1);

        const FileLocation *loc = m->loc;
        if (!loc || !loc->filename) {
            goto append_msg;
        }

        if (path_is_absolute(loc->filename)) {
            char *rel = path_relative(loc->filename, cwd);
            string_append_cstring(&buf, rel);
            free(rel);
        } else {
            string_append_cstring(&buf, loc->filename);
        }

        string_append_byte(&buf, ':');

        if (loc->pattern) {
            string_append_literal(&buf, "  /");
            string_append_cstring(&buf, loc->pattern);
            string_append_literal(&buf, "/\n");
            continue;
        }

        if (loc->line != 0) {
            string_append_cstring(&buf, ulong_to_str(loc->line));
            string_append_byte(&buf, ':');
            if (loc->column != 0) {
                string_append_cstring(&buf, ulong_to_str(loc->column));
                string_append_byte(&buf, ':');
            }
        }

        string_append_literal(&buf, "  ");

        append_msg:
        string_append_cstring(&buf, m->msg);
        string_append_byte(&buf, '\n');
    }

    return buf;
}
