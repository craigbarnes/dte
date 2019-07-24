#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "block.h"
#include "buffer.h"
#include "editor.h"
#include "file-option.h"
#include "filetype.h"
#include "lock.h"
#include "syntax/state.h"
#include "util/hashset.h"
#include "util/path.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/xmalloc.h"

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
void buffer_mark_lines_changed(Buffer *b, long min, long max)
{
    if (min > max) {
        long tmp = min;
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

Buffer *buffer_new(const Encoding *encoding)
{
    static unsigned int id;

    Buffer *b = xnew0(Buffer, 1);
    list_init(&b->blocks);
    b->cur_change = &b->change_head;
    b->saved_change = &b->change_head;
    b->id = ++id;
    b->newline = editor.options.newline;

    if (encoding) {
        b->encoding = *encoding;
    } else {
        b->encoding.type = ENCODING_AUTODETECT;
    }

    memcpy(&b->options, &editor.options, sizeof(CommonOptions));
    b->options.brace_indent = 0;
    b->options.filetype = str_intern("none");
    b->options.indent_regex = NULL;

    ptr_array_add(&buffers, b);
    return b;
}

Buffer *open_empty_buffer(void)
{
    Buffer *b = buffer_new(&editor.charset);

    // At least one block required
    Block *blk = block_new(1);
    list_add_before(&blk->node, &b->blocks);

    set_display_filename(b, xmemdup_literal("(No name)"));
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

Buffer *find_buffer_by_id(unsigned long id)
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
    const char *ft = NULL;
    if (BLOCK(b->blocks.next)->size) {
        BlockIter bi = BLOCK_ITER_INIT(&b->blocks);
        LineRef lr;
        fill_line_ref(&bi, &lr);
        const StringView line = string_view(lr.line, lr.size);
        ft = find_ft(b->abs_filename, line);
    } else if (b->abs_filename) {
        const StringView line = STRING_VIEW_INIT;
        ft = find_ft(b->abs_filename, line);
    }

    if (ft && !streq(ft, b->options.filetype)) {
        b->options.filetype = str_intern(ft);
        return true;
    }
    return false;
}

static char *short_filename_cwd(const char *absolute, const char *cwd)
{
    char *f = relative_filename(absolute, cwd);
    size_t home_len = strlen(editor.home_dir);
    size_t abs_len = strlen(absolute);
    size_t f_len = strlen(f);

    if (f_len >= abs_len) {
        // Prefer absolute if relative isn't shorter
        free(f);
        f = xstrdup(absolute);
        f_len = abs_len;
    }

    if (
        abs_len > home_len
        && !memcmp(absolute, editor.home_dir, home_len)
        && absolute[home_len] == '/'
    ) {
        size_t len = abs_len - home_len + 1;
        if (len < f_len) {
            char *filename = xmalloc(len + 1);
            filename[0] = '~';
            memcpy(filename + 1, absolute + home_len, len);
            free(f);
            return filename;
        }
    }
    return f;
}

char *short_filename(const char *absolute)
{
    char cwd[8192];
    if (getcwd(cwd, sizeof(cwd))) {
        return short_filename_cwd(absolute, cwd);
    }
    return xstrdup(absolute);
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

static bool allow_odd_indent(const Buffer *b)
{
    // 1, 3, 5 and 7 space indent
    const unsigned int odd = 1 << 0 | 1 << 2 | 1 << 4 | 1 << 6;
    return (b->options.detect_indent & odd) ? true : false;
}

static int indent_len(const Buffer *b, const char *line, int len, bool *tab_indent)
{
    bool space_before_tab = false;
    int spaces = 0;
    int tabs = 0;
    int pos = 0;

    while (pos < len) {
        if (line[pos] == ' ') {
            spaces++;
        } else if (line[pos] == '\t') {
            tabs++;
            if (spaces) {
                space_before_tab = true;
            }
        } else {
            break;
        }
        pos++;
    }
    *tab_indent = false;
    if (pos == len) {
        // Whitespace only
        return -1;
    }
    if (pos == 0) {
        // Not indented
        return 0;
    }
    if (space_before_tab) {
        // Mixed indent
        return -2;
    }
    if (tabs) {
        // Tabs and possible spaces after tab for alignment
        *tab_indent = true;
        return tabs * 8;
    }
    if (len > spaces && line[spaces] == '*') {
        // '*' after indent, could be long C style comment
        if (spaces % 2 || allow_odd_indent(b)) {
            return spaces - 1;
        }
    }
    return spaces;
}

static bool detect_indent(Buffer *b)
{
    BlockIter bi = BLOCK_ITER_INIT(&b->blocks);
    int current_indent = 0;
    int counts[9] = {0};
    int tab_count = 0;
    int space_count = 0;

    for (unsigned int i = 0; i < 200; i++) {
        LineRef lr;
        int indent;
        bool tab;

        fill_line_ref(&bi, &lr);
        indent = indent_len(b, lr.line, lr.size, &tab);
        if (indent == -2) {
            // Ignore mixed indent because tab width might not be 8
        } else if (indent == -1) {
            // Empty line, no change in indent
        } else if (indent == 0) {
            current_indent = 0;
        } else {
            // Indented line
            int change;

            // Count only increase in indentation because indentation
            // almost always grows one level at time, whereas it can
            // can decrease multiple levels all at once.
            if (current_indent == -1) {
                current_indent = 0;
            }
            change = indent - current_indent;
            if (change > 0 && change <= 8) {
                counts[change]++;
            }

            if (tab) {
                tab_count++;
            } else {
                space_count++;
            }
            current_indent = indent;
        }

        if (!block_iter_next_line(&bi)) {
            break;
        }
    }
    if (tab_count == 0 && space_count == 0) {
        return false;
    }
    if (tab_count > space_count) {
        b->options.emulate_tab = false;
        b->options.expand_tab = false;
        b->options.indent_width = b->options.tab_width;
    } else {
        size_t m = 0;
        for (size_t i = 1; i < ARRAY_COUNT(counts); i++) {
            if (b->options.detect_indent & 1 << (i - 1)) {
                if (counts[i] > counts[m]) {
                    m = i;
                }
            }
        }
        if (m == 0) {
            return false;
        }
        b->options.emulate_tab = true;
        b->options.expand_tab = true;
        b->options.indent_width = m;
    }
    return true;
}

void buffer_setup(Buffer *b)
{
    b->setup = true;
    buffer_detect_filetype(b);
    set_file_options(b);
    set_editorconfig_options(b);
    buffer_update_syntax(b);
    if (b->options.detect_indent && b->abs_filename != NULL) {
        detect_indent(b);
    }
}
