#include <stdbool.h>
#include <stdlib.h>
#include "bookmark.h"
#include "buffer.h"
#include "editor.h"
#include "move.h"
#include "search.h"
#include "selection.h"
#include "util/debug.h"

FileLocation *get_current_file_location(const View *v)
{
    const Buffer *b = v->buffer;
    char *filename = b->abs_filename ? xstrdup(b->abs_filename) : NULL;
    return new_file_location(filename, b->id, v->cy + 1, v->cx_char + 1);
}

bool file_location_go(Window *window, const FileLocation *loc)
{
    View *view = window_open_buffer(window, loc->filename, true, NULL);
    if (!view) {
        // Failed to open file; error message should be visible
        return false;
    }

    if (window->view != view) {
        set_view(view);
        // Force centering view to cursor, because file changed
        view->force_center = true;
    }

    if (loc->pattern) {
        if (!search_tag(view, loc->pattern)) {
            return false;
        }
    } else if (loc->line > 0) {
        move_to_filepos(view, loc->line, MAX(loc->column, 1));
    }

    return unselect(view);
}

static bool file_location_return(Window *window, const FileLocation *loc)
{
    Buffer *buffer = find_buffer_by_id(&window->editor->buffers, loc->buffer_id);
    View *view;
    if (buffer) {
        view = window_find_or_create_view(window, buffer);
    } else {
        if (!loc->filename) {
            // Can't restore closed buffer that had no filename; try again
            return false;
        }
        view = window_open_buffer(window, loc->filename, true, NULL);
    }

    if (!view) {
        // Open failed; don't try again
        return true;
    }

    set_view(view);
    move_to_filepos(view, loc->line, loc->column);
    return unselect(view);
}

void file_location_free(FileLocation *loc)
{
    free(loc->filename);
    free(loc->pattern);
    free(loc);
}

void bookmark_push(PointerArray *bookmarks, FileLocation *loc)
{
    const size_t max_entries = 256;
    if (bookmarks->count == max_entries) {
        file_location_free(ptr_array_remove_index(bookmarks, 0));
    }
    BUG_ON(bookmarks->count >= max_entries);
    ptr_array_append(bookmarks, loc);
}

void bookmark_pop(PointerArray *bookmarks, Window *window)
{
    void **ptrs = bookmarks->ptrs;
    size_t count = bookmarks->count;
    bool go = true;
    while (count > 0 && go) {
        FileLocation *loc = ptrs[--count];
        go = !file_location_return(window, loc);
        file_location_free(loc);
    }
    bookmarks->count = count;
}
