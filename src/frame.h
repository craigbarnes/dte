#ifndef FRAME_H
#define FRAME_H

#include <stdbool.h>
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string.h"

typedef struct Frame {
    struct Frame *parent;
    // Every frame contains either one window or multiple subframes
    PointerArray frames;
    struct Window *window;
    int w, h; // Width and height
    bool vertical;
    bool equal_size;
} Frame;

typedef enum {
    RESIZE_DIRECTION_AUTO,
    RESIZE_DIRECTION_HORIZONTAL,
    RESIZE_DIRECTION_VERTICAL,
} ResizeDirection;

struct EditorState;

Frame *new_root_frame(struct Window *w);
void set_frame_size(Frame *frame, int w, int h);
void equalize_frame_sizes(Frame *parent);
void add_to_frame_size(Frame *frame, ResizeDirection dir, int amount);
void resize_frame(Frame *frame, ResizeDirection dir, int size);
void update_window_coordinates(Frame *frame);
Frame *split_frame(struct Window *w, bool vertical, bool before);
Frame *split_root(struct EditorState *e, bool vertical, bool before);
void remove_frame(struct EditorState *e, Frame *frame);
void dump_frame(const Frame *frame, int level, String *str);

#if DEBUG >= 1
  void debug_frame(const Frame *frame);
#else
  static inline void debug_frame(const Frame* UNUSED_ARG(frame)) {}
#endif

#endif
