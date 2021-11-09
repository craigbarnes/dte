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

static PointerArray file_locations = PTR_ARRAY_INIT;

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
    Window *w = window;
    View *v = window_open_buffer(w, loc->filename, true, NULL);
    if (!v) {
        // Failed to open file. Error message should be visible.
        return false;
    }

    if (w->view != v) {
        set_view(v);
        // Force centering view to the cursor because file changed
        v->force_center = true;
    }

    bool ok = true;
    if (loc->pattern) {
        bool err = false;
        search_tag(v, loc->pattern, &err);
        ok = !err;
    } else if (loc->line > 0) {
        move_to_line(v, loc->line);
        if (loc->column > 0) {
            move_to_column(v, loc->column);
        }
    }
    if (ok) {
        unselect(v);
    }
    return ok;
}

static bool file_location_return(const FileLocation *loc)
{
    Window *w = window;
    Buffer *b = find_buffer_by_id(loc->buffer_id);
    View *v;

    if (b) {
        v = window_get_view(w, b);
    } else {
        if (!loc->filename) {
            // Can't restore closed buffer that had no filename.
            // Try again.
            return false;
        }
        v = window_open_buffer(w, loc->filename, true, NULL);
    }

    if (!v) {
        // Open failed. Don't try again.
        return true;
    }

    set_view(v);
    unselect(v);
    move_to_line(v, loc->line);
    move_to_column(v, loc->column);
    return true;
}

void push_file_location(FileLocation *loc)
{
    const size_t max_entries = 256;
    if (file_locations.count == max_entries) {
        file_location_free(ptr_array_remove_idx(&file_locations, 0));
    }
    BUG_ON(file_locations.count >= max_entries);
    ptr_array_append(&file_locations, loc);
}

void pop_file_location(void)
{
    bool go = true;
    while (file_locations.count > 0 && go) {
        FileLocation *loc = file_locations.ptrs[--file_locations.count];
        go = !file_location_return(loc);
        file_location_free(loc);
    }
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

void activate_current_message_save(const MessageArray *arr, const View *view)
{
    const BlockIter save = view->cursor;
    FileLocation *loc = get_current_file_location(view);
    activate_current_message(arr);

    // Save position if file changed or cursor moved
    view = editor.view;
    if (view->cursor.blk != save.blk || view->cursor.offset != save.offset) {
        push_file_location(loc);
    } else {
        file_location_free(loc);
    }
}

void clear_messages(MessageArray *msgs)
{
    ptr_array_free_cb(&msgs->array, FREE_FUNC(free_message));
    msgs->pos = 0;
}

String dump_messages(void)
{
    String buf = string_new(4096);
    char cwd[8192];
    if (unlikely(!getcwd(cwd, sizeof cwd))) {
        return buf;
    }

    for (size_t i = 0, n = editor.messages.array.count; i < n; i++) {
        const Message *m = editor.messages.array.ptrs[i];
        string_sprintf(&buf, "%zu: ", i + 1);

        const FileLocation *loc = m->loc;
        if (!loc || !loc->filename) {
            goto append_msg;
        }

        if (path_is_absolute(loc->filename)) {
            char *rel = relative_filename(loc->filename, cwd);
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
