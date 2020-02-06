#ifndef BUFFER_H
#define BUFFER_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include "change.h"
#include "encoding/encoding.h"
#include "options.h"
#include "syntax/syntax.h"
#include "util/list.h"
#include "util/macros.h"
#include "util/ptr-array.h"

// Subset of stat(3) struct
typedef struct {
    off_t size;
    mode_t mode;
    gid_t gid;
    uid_t uid;
    time_t mtime_sec;
    long mtime_nsec;
    dev_t dev;
    ino_t ino;
} FileInfo;

typedef struct Buffer {
    ListHead blocks;
    Change change_head;
    Change *cur_change;

    // Used to determine if buffer is modified
    Change *saved_change;

    FileInfo file;

    // Needed for identifying buffers whose filename is NULL
    unsigned long id;

    size_t nl;

    // Views pointing to this buffer
    PointerArray views;

    char *display_filename;
    char *abs_filename;

    bool readonly;
    bool temporary;
    bool locked;
    bool setup;
    bool crlf_newlines;

    // Encoding of the file. Buffer always contains UTF-8.
    Encoding encoding;

    LocalOptions options;

    Syntax *syn;
    // Index 0 is always syn->states.ptrs[0].
    // Lowest bit of an invalidated value is 1.
    PointerArray line_start_states;

    long changed_line_min;
    long changed_line_max;
} Buffer;

// buffer = view->buffer = window->view->buffer
extern struct View *view;
extern Buffer *buffer;

static inline void mark_all_lines_changed(Buffer *b)
{
    b->changed_line_min = 0;
    b->changed_line_max = INT_MAX;
}

static inline bool buffer_modified(const Buffer *b)
{
    return b->saved_change != b->cur_change && !b->temporary;
}

void buffer_mark_lines_changed(Buffer *b, long min, long max);
const char *buffer_filename(const Buffer *b);

char *short_filename(const char *absolute) XSTRDUP;
void update_short_filename_cwd(Buffer *b, const char *cwd);
void update_short_filename(Buffer *b);
bool buffer_stat(Buffer *b, const char *filename);
bool buffer_fstat(Buffer *b, int fd);
Buffer *find_buffer(const char *abs_filename);
Buffer *find_buffer_by_id(unsigned long id);
Buffer *buffer_new(const Encoding *encoding);
Buffer *open_empty_buffer(void);
void free_buffer(Buffer *b);
bool buffer_detect_filetype(Buffer *b);
void buffer_update_syntax(Buffer *b);
void buffer_setup(Buffer *b);

#endif
