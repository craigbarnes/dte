#include <stdlib.h>
#include "bookmark.h"
#include "buffer.h"
#include "editor.h"
#include "misc.h"
#include "move.h"
#include "search.h"
#include "util/xmalloc.h"
#include "window.h"

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

bool file_location_go(const FileLocation *loc)
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

void file_location_free(FileLocation *loc)
{
    free(loc->filename);
    free(loc->pattern);
    free(loc);
}
