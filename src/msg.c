#include <stdbool.h>
#include <stdlib.h>
#include "msg.h"
#include "buffer.h"
#include "edit.h"
#include "error.h"
#include "move.h"
#include "search.h"
#include "util/debug.h"
#include "util/ptr-array.h"
#include "util/xmalloc.h"
#include "view.h"
#include "window.h"

static PointerArray file_locations = PTR_ARRAY_INIT;
static PointerArray msgs = PTR_ARRAY_INIT;
static size_t msg_pos;

void file_location_free(FileLocation *loc)
{
    free(loc->filename);
    free(loc->pattern);
    free(loc);
}

FileLocation *get_current_file_location(void)
{
    const char *filename = buffer->abs_filename;
    FileLocation *loc = xmalloc(sizeof(*loc));
    *loc = (FileLocation) {
        .filename = filename ? xstrdup(filename) : NULL,
        .buffer_id = buffer->id,
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
        search_tag(loc->pattern, &err);
        ok = !err;
    } else if (loc->line > 0) {
        move_to_line(v, loc->line);
        if (loc->column > 0) {
            move_to_column(v, loc->column);
        }
    }
    if (ok) {
        unselect();
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
    unselect();
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

void add_message(Message *m)
{
    ptr_array_append(&msgs, m);
}

void activate_current_message(void)
{
    if (msg_pos == msgs.count) {
        return;
    }
    const Message *m = msgs.ptrs[msg_pos];
    if (m->loc && m->loc->filename) {
        if (!file_location_go(m->loc)) {
            // Error message is visible
            return;
        }
    }
    if (msgs.count == 1) {
        info_msg("%s", m->msg);
    } else {
        info_msg("[%zu/%zu] %s", msg_pos + 1, msgs.count, m->msg);
    }
}

void activate_next_message(void)
{
    if (msg_pos + 1 < msgs.count) {
        msg_pos++;
    }
    activate_current_message();
}

void activate_prev_message(void)
{
    if (msg_pos > 0) {
        msg_pos--;
    }
    activate_current_message();
}

void activate_current_message_save(void)
{
    const BlockIter save = view->cursor;
    FileLocation *loc = get_current_file_location();
    activate_current_message();

    // Save position if file changed or cursor moved
    if (view->cursor.blk != save.blk || view->cursor.offset != save.offset) {
        push_file_location(loc);
    } else {
        file_location_free(loc);
    }
}

void clear_messages(void)
{
    ptr_array_free_cb(&msgs, FREE_FUNC(free_message));
    msg_pos = 0;
}

size_t message_count(void)
{
    return msgs.count;
}
