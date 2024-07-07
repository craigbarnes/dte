#ifndef BUFFER_H
#define BUFFER_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include "block-iter.h"
#include "change.h"
#include "options.h"
#include "syntax/syntax.h"
#include "util/debug.h"
#include "util/list.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string-view.h"
#include "util/string.h"

/*
 * Subset of stat(3) struct, to minimize size bloat.
 * Example sizes (glibc 2.38, x86_64):
 *
 * • sizeof(FileInfo): 56
 * • sizeof(struct stat): 144
 * • Difference: 88 bytes (per Buffer)
 */
typedef struct {
    dev_t dev;
    ino_t ino;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    off_t size;
    struct timespec mtime;
} FileInfo;

/*
 * A representation of a specific file, as it pertains to editing,
 * including text contents, filename (if saved), undo history and
 * some file-specific metadata and options.
 */
typedef struct Buffer {
    ListHead blocks; // Doubly linked list of Blocks, forming the text contents
    Change change_head;
    Change *cur_change;
    Change *saved_change; // Used to determine if there are unsaved changes
    FileInfo file; // File metadata, taken from the most recent stat(3) call
    unsigned long id; // Needed for identifying buffers whose filename is NULL
    size_t nl; // Total number of lines (sum of all Block::nl counts)
    PointerArray views; // Views pointing to this buffer
    char *display_filename; // Short filename, as displayed in the tab bar
    char *abs_filename;
    bool readonly;
    bool temporary;
    bool stdout_buffer;
    bool locked;
    bool setup;
    bool crlf_newlines;
    bool bom;
    const char *encoding; // Encoding of the file (buffer always contains UTF-8)
    LocalOptions options;
    Syntax *syntax;
    long changed_line_min;
    long changed_line_max;
    // Index 0 is always syn->states.ptrs[0].
    // Lowest bit of an invalidated value is 1.
    PointerArray line_start_states;
} Buffer;

static inline void mark_all_lines_changed(Buffer *buffer)
{
    buffer->changed_line_min = 0;
    buffer->changed_line_max = LONG_MAX;
}

static inline bool buffer_modified(const Buffer *buffer)
{
    return buffer->saved_change != buffer->cur_change && !buffer->temporary;
}

static inline struct View *buffer_get_first_view(const Buffer *buffer)
{
    BUG_ON(buffer->views.count == 0);
    return buffer->views.ptrs[0];
}

static inline BlockIter block_iter(Buffer *buffer)
{
    return (BlockIter) {
        .blk = BLOCK(buffer->blocks.next),
        .head = &buffer->blocks,
        .offset = 0
    };
}

struct EditorState;

const char *buffer_filename(const Buffer *buffer) NONNULL_ARGS_AND_RETURN;
void buffer_mark_lines_changed(Buffer *buffer, long min, long max) NONNULL_ARGS;
void buffer_set_encoding(Buffer *buffer, const char *encoding, bool utf8_bom) NONNULL_ARGS;
void buffer_set_display_filename(Buffer *buffer, char *name) NONNULL_ARG(1);
void buffer_update_short_filename_cwd(Buffer *buffer, const StringView *home, const char *cwd) NONNULL_ARG(1, 2);
void buffer_update_short_filename(Buffer *buffer, const StringView *home) NONNULL_ARGS;
bool buffer_detect_filetype(Buffer *buffer, const PointerArray *filetypes) NONNULL_ARGS;
void buffer_update_syntax(struct EditorState *e, Buffer *buffer) NONNULL_ARGS;
void buffer_setup(struct EditorState *e, Buffer *buffer) NONNULL_ARGS;
void buffer_count_blocks_and_bytes(const Buffer *buffer, uintmax_t counts[2]) NONNULL_ARGS;
void free_buffer(Buffer *buffer) NONNULL_ARGS;
void free_blocks(Buffer *buffer) NONNULL_ARGS;
String dump_buffer(const Buffer *buffer) NONNULL_ARGS;

Buffer *find_buffer(const PointerArray *buffers, const char *abs_filename) NONNULL_ARGS;
Buffer *find_buffer_by_id(const PointerArray *buffers, unsigned long id) NONNULL_ARGS;
Buffer *buffer_new(PointerArray *buffers, const GlobalOptions *gopts, const char *encoding) RETURNS_NONNULL NONNULL_ARG(1, 2);
Buffer *open_empty_buffer(PointerArray *buffers, const GlobalOptions *gopts) NONNULL_ARGS_AND_RETURN;
void remove_and_free_buffer(PointerArray *buffers, Buffer *buffer) NONNULL_ARGS;

#endif
