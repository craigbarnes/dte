#include "buffer.h"
#include "view.h"
#include "editor.h"
#include "block.h"
#include "filetype.h"
#include "state.h"
#include "file-option.h"
#include "lock.h"
#include "selection.h"
#include "path.h"
#include "unicode.h"
#include "uchar.h"
#include "detect.h"

Buffer *buffer;
PointerArray buffers = PTR_ARRAY_INIT;

static void set_display_filename(Buffer *b, char *name)
{
    free(b->display_filename);
    b->display_filename = name;
}

/*
 * Mark line range min...max (inclusive) "changed". These lines will be
 * redrawn when screen is updated. This is called even when content has not
 * been changed, but selection has or line has been deleted and all lines
 * after the deleted line move up.
 *
 * Syntax highlighter has different logic. It cares about contents of the
 * lines, not about selection or if the lines have been moved up or down.
 */
void buffer_mark_lines_changed(Buffer *b, int min, int max)
{
    if (min > max) {
        int tmp = min;
        min = max;
        max = tmp;
    }
    if (min < b->changed_line_min) {
        b->changed_line_min = min;
    }
    if (max > b->changed_line_max) {
        b->changed_line_max = max;
    }
}

const char *buffer_filename(const Buffer *b)
{
    return b->display_filename;
}

Buffer *buffer_new(const char *encoding)
{
    static int id;

    Buffer *b = xnew0(Buffer, 1);
    list_init(&b->blocks);
    b->cur_change = &b->change_head;
    b->saved_change = &b->change_head;
    b->id = ++id;
    b->newline = editor.options.newline;
    if (encoding) {
        b->encoding = xstrdup(encoding);
    }

    memcpy(&b->options, &editor.options, sizeof(CommonOptions));
    b->options.brace_indent = 0;
    b->options.filetype = xstrdup("none");
    b->options.indent_regex = xstrdup("");

    ptr_array_add(&buffers, b);
    return b;
}

Buffer *open_empty_buffer(void)
{
    Buffer *b = buffer_new(editor.charset);

    // At least one block required
    Block *blk = block_new(1);
    list_add_before(&blk->node, &b->blocks);

    set_display_filename(b, xstrdup("(No name)"));
    return b;
}

void free_buffer(Buffer *b)
{
    ptr_array_remove(&buffers, b);

    if (b->locked) {
        unlock_file(b->abs_filename);
    }

    ListHead *item = b->blocks.next;
    while (item != &b->blocks) {
        ListHead *next = item->next;
        Block *blk = BLOCK(item);

        free(blk->data);
        free(blk);
        item = next;
    }
    free_changes(&b->change_head);
    free(b->line_start_states.ptrs);
    free(b->views.ptrs);
    free(b->display_filename);
    free(b->abs_filename);
    free(b->encoding);
    free_local_options(&b->options);
    free(b);
}

static bool same_file(const struct stat *const a, const struct stat *const b)
{
    return a->st_dev == b->st_dev && a->st_ino == b->st_ino;
}

Buffer *find_buffer(const char *abs_filename)
{
    struct stat st;
    bool st_ok = stat(abs_filename, &st) == 0;

    for (size_t i = 0; i < buffers.count; i++) {
        Buffer *b = buffers.ptrs[i];
        const char *f = b->abs_filename;

        if (
            (f != NULL && streq(f, abs_filename))
            || (st_ok && same_file(&st, &b->st))
        ) {
            return b;
        }
    }
    return NULL;
}

Buffer *find_buffer_by_id(unsigned int id)
{
    for (size_t i = 0; i < buffers.count; i++) {
        Buffer *b = buffers.ptrs[i];
        if (b->id == id) {
            return b;
        }
    }
    return NULL;
}

bool buffer_detect_filetype(Buffer *b)
{
    char *interpreter = detect_interpreter(b);
    const char *ft = NULL;

    if (BLOCK(b->blocks.next)->size) {
        BlockIter bi = BLOCK_ITER_INIT(&b->blocks);
        LineRef lr;

        fill_line_ref(&bi, &lr);
        ft = find_ft(b->abs_filename, interpreter, lr.line, lr.size);
    } else if (b->abs_filename) {
        ft = find_ft(b->abs_filename, interpreter, NULL, 0);
    }
    free(interpreter);

    if (ft && !streq(ft, b->options.filetype)) {
        free(b->options.filetype);
        b->options.filetype = xstrdup(ft);
        return true;
    }
    return false;
}

void update_short_filename_cwd(Buffer *b, const char *cwd)
{
    if (b->abs_filename) {
        if (cwd) {
            set_display_filename(b, short_filename_cwd(b->abs_filename, cwd));
        } else {
            // getcwd() failed
            set_display_filename(b, xstrdup(b->abs_filename));
        }
    }
}

void update_short_filename(Buffer *b)
{
    set_display_filename(b, short_filename(b->abs_filename));
}

void buffer_update_syntax(Buffer *b)
{
    Syntax *syn = NULL;

    if (b->options.syntax) {
        // Even "none" can have syntax
        syn = find_syntax(b->options.filetype);
        if (!syn) {
            syn = load_syntax_by_filetype(b->options.filetype);
        }
    }
    if (syn == b->syn) {
        return;
    }

    b->syn = syn;
    if (syn) {
        // Start state of first line is constant
        PointerArray *s = &b->line_start_states;
        if (!s->alloc) {
            s->alloc = 64;
            s->ptrs = xnew(void *, s->alloc);
        }
        s->ptrs[0] = syn->states.ptrs[0];
        s->count = 1;
    }

    mark_all_lines_changed(b);
}

void buffer_setup(Buffer *b)
{
    b->setup = true;
    buffer_detect_filetype(b);
    set_file_options(b);
    buffer_update_syntax(b);
    if (b->options.detect_indent && b->abs_filename != NULL) {
        detect_indent(b);
    }
}
