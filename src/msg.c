#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
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

void activate_current_message(const MessageArray *msgs)
{
    size_t pos = msgs->pos;
    size_t count = msgs->array.count;
    if (pos == count) {
        return;
    }

    const Message *m = msgs->array.ptrs[pos];
    if (m->loc && m->loc->filename) {
        if (!file_location_go(m->loc)) {
            // Error message is visible
            return;
        }
    }

    if (count == 1) {
        info_msg("%s", m->msg);
    } else {
        info_msg("[%zu/%zu] %s", pos + 1, count, m->msg);
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

void activate_current_message_save(const MessageArray *msgs, PointerArray *file_locations, const View *view)
{
    const BlockIter save = view->cursor;
    FileLocation *loc = get_current_file_location(view);
    activate_current_message(msgs);

    // Save position if file changed or cursor moved
    view = editor.view;
    if (view->cursor.blk != save.blk || view->cursor.offset != save.offset) {
        bookmark_push(file_locations, loc);
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
