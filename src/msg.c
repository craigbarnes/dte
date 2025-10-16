#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "msg.h"
#include "command/error.h"
#include "editor.h"
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
    Message *m = xmalloc(xadd3(sizeof(*m), len, 1));
    m->loc = NULL;
    if (len) {
        memcpy(m->msg, msg, len);
    }
    m->msg[len] = '\0';
    return m;
}

void add_message(MessageList *msgs, Message *m)
{
    ptr_array_append(&msgs->array, m);
}

// Jump to the FileLocation of the current Message
bool activate_current_message(const MessageList *msgs, Window *window)
{
    size_t count = msgs->array.count;
    if (count == 0) {
        return false;
    }

    size_t pos = msgs->pos;
    BUG_ON(pos >= count);
    const Message *m = msgs->array.ptrs[pos];
    const FileLocation *loc = m->loc;
    if (loc && loc->filename && !file_location_go(window, loc)) {
        // Failed to jump to location; error message is visible
        return false;
    }

    ErrorBuffer *ebuf = &window->editor->err;
    if (count == 1) {
        return info_msg(ebuf, "%s", m->msg);
    }

    return info_msg(ebuf, "[%zu/%zu] %s", pos + 1, count, m->msg);
}

// Like activate_current_message(), but also pushing the previous
// FileLocation on the bookmark stack if the cursor moves
void activate_current_message_save (
    const MessageList *msgs,
    PointerArray *bookmarks,
    const View *view
) {
    size_t nmsgs = msgs->array.count;
    if (nmsgs == 0) {
        return;
    }

    const BlockIter save = view->cursor;
    const unsigned long line = view->cy + 1;
    const unsigned long col = view->cx_char + 1;
    activate_current_message(msgs, view->window);

    const BlockIter *cursor = &view->window->editor->view->cursor;
    if (nmsgs == 1 && cursor->blk == save.blk && cursor->offset == save.offset) {
        // Only one message and cursor position not changed; don't bookmark.
        // TODO: Make this condition configurable (some people may prefer
        // to *always* push a bookmark)
        return;
    }

    // Active view or cursor position changed, or MAY change due to
    // there being multiple Messages to navigate with `msg -n|-p`;
    // bookmark the previous location
    const Buffer *b = view->buffer;
    char *filename = b->abs_filename ? xstrdup(b->abs_filename) : NULL;
    bookmark_push(bookmarks, new_file_location(filename, b->id, line, col));
}

void clear_messages(MessageList *msgs)
{
    msgs->pos = 0;
    ptr_array_free_cb(&msgs->array, FREE_FUNC(free_message));
}

String dump_messages(const MessageList *messages)
{
    size_t count = messages->array.count;
    char cwd[8192];
    if (unlikely(!getcwd(cwd, sizeof cwd)) || count == 0) {
        return string_new(0);
    }

    String buf = string_new(4096);
    for (size_t i = 0; i < count; i++) {
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
