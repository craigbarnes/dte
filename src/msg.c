#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "msg.h"
#include "error.h"
#include "util/debug.h"
#include "util/numtostr.h"
#include "util/path.h"
#include "util/xmalloc.h"

static void free_message(Message *m)
{
    if (m->loc) {
        file_location_free(m->loc);
    }
    free(m);
}

Message *new_message(const char *msg, size_t len)
{
    Message *m = xmalloc(sizeof(*m) + len + 1);
    m->loc = NULL;
    if (len) {
        memcpy(m->msg, msg, len);
    }
    m->msg[len] = '\0';
    return m;
}

void add_message(MessageArray *msgs, Message *m)
{
    ptr_array_append(&msgs->array, m);
}

View *activate_current_message(const MessageArray *msgs, Window *window)
{
    size_t count = msgs->array.count;
    if (count == 0) {
        return NULL;
    }

    size_t pos = msgs->pos;
    BUG_ON(pos >= count);
    const Message *m = msgs->array.ptrs[pos];
    const FileLocation *loc = m->loc;
    View *view = NULL;

    if (loc && loc->filename) {
        view = file_location_go(window, loc);
        if (!view) {
            // Failed to jump to location; error message is visible
            return NULL;
        }
    }

    if (count == 1) {
        info_msg("%s", m->msg);
    } else {
        info_msg("[%zu/%zu] %s", pos + 1, count, m->msg);
    }

    return view;
}

void activate_current_message_save (
    const MessageArray *msgs,
    PointerArray *bookmarks,
    const View *view
) {
    if (msgs->array.count == 0) {
        return;
    }

    const BlockIter save = view->cursor;
    FileLocation *loc = get_current_file_location(view);
    view = activate_current_message(msgs, view->window);

    if (view) {
        const BlockIter *cursor = &view->cursor;
        if (cursor->blk != save.blk || cursor->offset != save.offset) {
            // Bookmark previous location if file changed or cursor moved
            bookmark_push(bookmarks, loc);
            return;
        }
    }

    file_location_free(loc);
}

void clear_messages(MessageArray *msgs)
{
    msgs->pos = 0;
    ptr_array_free_cb(&msgs->array, FREE_FUNC(free_message));
}

String dump_messages(const MessageArray *messages)
{
    String buf = string_new(4096);
    char cwd[8192];
    if (unlikely(!getcwd(cwd, sizeof cwd))) {
        return buf;
    }

    for (size_t i = 0, n = messages->array.count; i < n; i++) {
        char *ptr = string_reserve_space(&buf, DECIMAL_STR_MAX(i));
        buf.len += buf_umax_to_str(i + 1, ptr);
        string_append_literal(&buf, ": ");

        const Message *m = messages->array.ptrs[i];
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
