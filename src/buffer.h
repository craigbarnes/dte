#ifndef BUFFER_H
#define BUFFER_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include "change.h"
#include "encoding.h"
#include "options.h"
#include "syntax/syntax.h"
#include "util/list.h"
#include "util/macros.h"
#include "util/ptr-array.h"

// Subset of stat(3) struct
typedef struct {
    dev_t dev;
    ino_t ino;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    off_t size;
    time_t mtime;
} FileInfo;

typedef struct Buffer {
    ListHead blocks;
    Change change_head;
    Change *cur_change;
    Change *saved_change; // Used to determine if buffer is modified
    FileInfo file;
    unsigned long id; // Needed for identifying buffers whose filename is NULL
    size_t nl;
    PointerArray views; // Views pointing to this buffer
    char *display_filename;
    char *abs_filename;
    bool readonly;
    bool temporary;
    bool stdout_buffer;
    bool locked;
    bool setup;
    bool crlf_newlines;
    bool bom;
    Encoding encoding; // Encoding of the file (buffer always contains UTF-8)
    LocalOptions options;
    Syntax *syn;
    long changed_line_min;
    long changed_line_max;
    // Index 0 is always syn->states.ptrs[0].
    // Lowest bit of an invalidated value is 1.
    PointerArray line_start_states;
} Buffer;

static inline void mark_all_lines_changed(Buffer *b)
{
    b->changed_line_min = 0;
    b->changed_line_max = LONG_MAX;
}

static inline bool use_spaces_for_indent(const Buffer *b)
{
    const LocalOptions *opt = &b->options;
    return opt->expand_tab || opt->indent_width != opt->tab_width;
}

static inline bool buffer_modified(const Buffer *b)
{
    return b->saved_change != b->cur_change && !b->temporary;
}

void buffer_mark_lines_changed(Buffer *b, long min, long max) NONNULL_ARGS;
void buffer_set_encoding(Buffer *b, Encoding encoding) NONNULL_ARGS;
const char *buffer_filename(const Buffer *b) NONNULL_ARGS_AND_RETURN;
void set_display_filename(Buffer *b, char *name) NONNULL_ARG(1);
void update_short_filename_cwd(Buffer *b, const char *cwd) NONNULL_ARG(1);
void update_short_filename(Buffer *b) NONNULL_ARGS;
Buffer *find_buffer(const PointerArray *buffers, const char *abs_filename) NONNULL_ARGS;
Buffer *find_buffer_by_id(const PointerArray *buffers, unsigned long id) NONNULL_ARGS;
Buffer *buffer_new(const Encoding *encoding) RETURNS_NONNULL;
Buffer *open_empty_buffer(void) RETURNS_NONNULL;
void free_buffer(PointerArray *buffers, Buffer *b) NONNULL_ARGS;
void free_blocks(Buffer *b) NONNULL_ARGS;
bool buffer_detect_filetype(Buffer *b) NONNULL_ARGS;
void buffer_update_syntax(Buffer *b) NONNULL_ARGS;
void buffer_setup(Buffer *b, const PointerArray *file_options) NONNULL_ARGS;

#endif
