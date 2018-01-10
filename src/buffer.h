#ifndef BUFFER_H
#define BUFFER_H

#include <sys/stat.h>
#include <stdbool.h>
#include <limits.h>
#include "iter.h"
#include "list.h"
#include "options.h"
#include "common.h"
#include "ptr-array.h"
#include "change.h"
#include "syntax.h"
#include "editor.h"

typedef struct Buffer {
    ListHead blocks;
    Change change_head;
    Change *cur_change;

    // Used to determine if buffer is modified
    Change *saved_change;

    struct stat st;

    // Needed for identifying buffers whose filename is NULL
    unsigned int id;

    long nl;

    // Views pointing to this buffer
    PointerArray views;

    char *display_filename;
    char *abs_filename;

    bool ro;
    bool locked;
    bool setup;

    LineEndingType newline;

    // Encoding of the file. Buffer always contains UTF-8.
    char *encoding;

    LocalOptions options;

    Syntax *syn;
    // Index 0 is always syn->states.ptrs[0].
    // Lowest bit of an invalidated value is 1.
    PointerArray line_start_states;

    int changed_line_min;
    int changed_line_max;
} Buffer;

// buffer = view->buffer = window->view->buffer
extern struct View *view;
extern Buffer *buffer;
extern PointerArray buffers;

static inline void mark_all_lines_changed(Buffer *b)
{
    b->changed_line_min = 0;
    b->changed_line_max = INT_MAX;
}

static inline void mark_everything_changed(void)
{
    editor.everything_changed = true;
}

static inline bool buffer_modified(const Buffer *b)
{
    return b->saved_change != b->cur_change;
}

void buffer_mark_lines_changed(Buffer *b, int min, int max);
const char *buffer_filename(const Buffer *b);

void update_short_filename_cwd(Buffer *b, const char *cwd);
void update_short_filename(Buffer *b);
Buffer *find_buffer(const char *abs_filename);
Buffer *find_buffer_by_id(unsigned int id);
Buffer *buffer_new(const char *encoding);
Buffer *open_empty_buffer(void);
void free_buffer(Buffer *b);
bool buffer_detect_filetype(Buffer *b);
void buffer_update_syntax(Buffer *b);
void buffer_setup(Buffer *b);

long buffer_get_char(BlockIter *bi, unsigned int *up);
long buffer_next_char(BlockIter *bi, unsigned int *up);
long buffer_prev_char(BlockIter *bi, unsigned int *up);

#endif
