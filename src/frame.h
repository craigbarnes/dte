#ifndef FRAME_H
#define FRAME_H

#include <stdbool.h>
#include <stddef.h>
#include "util/debug.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string.h"

// A container for other Frames or Windows. Frames and Windows form
// a tree structure, wherein Windows are the terminal (leaf) nodes.
typedef struct Frame {
    struct Frame *parent;
    // Every frame contains either one window or multiple subframes
    PointerArray frames;
    struct Window *window;
    int w; // Width
    int h; // Height
    bool vertical;
    bool equal_size;
} Frame;

typedef enum {
    RESIZE_DIRECTION_AUTO,
    RESIZE_DIRECTION_HORIZONTAL,
    RESIZE_DIRECTION_VERTICAL,
} ResizeDirection;

struct EditorState;

Frame *new_root_frame(struct Window *window);
void frame_set_size(Frame *frame, int w, int h);
void frame_equalize_sizes(Frame *parent);
void frame_add_to_size(Frame *frame, ResizeDirection dir, int amount);
void frame_resize(Frame *frame, ResizeDirection dir, int size);
Frame *frame_split(struct Window *window, bool vertical, bool before);
Frame *frame_split_root(struct EditorState *e, bool vertical, bool before);
void frame_remove(struct EditorState *e, Frame *frame);
void dump_frame(const Frame *frame, size_t level, String *str);
void update_window_coordinates(Frame *frame);

#if DEBUG_ASSERTIONS_ENABLED
    void frame_debug(const Frame *frame);
#else
    static inline void frame_debug(const Frame* UNUSED_ARG(frame)) {}
#endif

#endif
