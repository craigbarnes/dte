#include "msg.h"
#include "buffer.h"
#include "error.h"
#include "move.h"
#include "search.h"
#include "util/ptr-array.h"
#include "util/str-util.h"
#include "util/xmalloc.h"
#include "window.h"

static PointerArray file_locations = PTR_ARRAY_INIT;
static PointerArray msgs = PTR_ARRAY_INIT;
static size_t msg_pos;

FileLocation *file_location_create (
    const char *filename,
    unsigned long buffer_id,
    unsigned long line,
    unsigned long column
) {
    FileLocation *loc = xnew0(FileLocation, 1);
    loc->filename = filename ? xstrdup(filename) : NULL;
    loc->buffer_id = buffer_id;
    loc->line = line;
    loc->column = column;
    return loc;
}

void file_location_free(FileLocation *loc)
{
    free(loc->filename);
    free(loc->pattern);
    free(loc);
}

static bool file_location_equals(const FileLocation *a, const FileLocation *b)
{
    if (!xstreq(a->filename, b->filename)) {
        return false;
    }
    if (a->buffer_id != b->buffer_id) {
        return false;
    }
    if (!xstreq(a->pattern, b->pattern)) {
        return false;
    }
    if (a->line != b->line) {
        return false;
    }
    if (a->column != b->column) {
        return false;
    }
    return true;
}

static bool file_location_go(const FileLocation *loc)
{
    Window *w = window;
    View *v = window_open_buffer(w, loc->filename, true, NULL);
    bool ok = true;

    if (!v) {
        // Failed to open file. Error message should be visible.
        return false;
    }
    if (w->view != v) {
        set_view(v);
        // Force centering view to the cursor because file changed
        v->force_center = true;
    }
    if (loc->pattern != NULL) {
        bool err = false;
        search_tag(loc->pattern, &err);
        ok = !err;
    } else if (loc->line > 0) {
        move_to_line(v, loc->line);
        if (loc->column > 0) {
            move_to_column(v, loc->column);
        }
    }
    return ok;
}

static bool file_location_return(const FileLocation *loc)
{
    Window *w = window;
    Buffer *b = find_buffer_by_id(loc->buffer_id);
    View *v;

    if (b != NULL) {
        v = window_get_view(w, b);
    } else {
        if (loc->filename == NULL) {
            // Can't restore closed buffer that had no filename.
            // Try again.
            return false;
        }
        v = window_open_buffer(w, loc->filename, true, NULL);
    }
    if (v == NULL) {
        // Open failed. Don't try again.
        return true;
    }
    set_view(v);
    move_to_line(v, loc->line);
    move_to_column(v, loc->column);
    return true;
}

void push_file_location(FileLocation *loc)
{
    ptr_array_add(&file_locations, loc);
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
    if (m->loc != NULL) {
        file_location_free(m->loc);
    }
    free(m);
}

static bool message_equals(const Message *a, const Message *b)
{
    if (!streq(a->msg, b->msg)) {
        return false;
    }
    if (a->loc == NULL) {
        return b->loc == NULL;
    }
    if (b->loc == NULL) {
        return false;
    }
    return file_location_equals(a->loc, b->loc);
}

static bool is_duplicate(const Message *m)
{
    for (size_t i = 0; i < msgs.count; i++) {
        if (message_equals(m, msgs.ptrs[i])) {
            return true;
        }
    }
    return false;
}

Message *new_message(const char *msg)
{
    Message *m = xnew0(Message, 1);
    m->msg = xstrdup(msg);
    return m;
}

void add_message(Message *m)
{
    if (is_duplicate(m)) {
        free_message(m);
    } else {
        ptr_array_add(&msgs, m);
    }
}

void activate_current_message(void)
{
    if (msg_pos == msgs.count) {
        return;
    }
    const Message *m = msgs.ptrs[msg_pos];
    if (m->loc != NULL && m->loc->filename != NULL) {
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

void clear_messages(void)
{
    ptr_array_free_cb(&msgs, FREE_FUNC(free_message));
    msg_pos = 0;
}

size_t message_count(void)
{
    return msgs.count;
}
