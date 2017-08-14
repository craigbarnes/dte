#ifndef FRAME_H
#define FRAME_H

#include "ptr-array.h"
#include "libc.h"

struct frame {
    struct frame *parent;

    // Every frame contains either one window or multiple subframes
    PointerArray frames;
    struct window *window;

    // Width and height
    int w, h;

    bool vertical;
    bool equal_size;
};

typedef struct frame Frame;

typedef enum {
    RESIZE_DIRECTION_AUTO,
    RESIZE_DIRECTION_HORIZONTAL,
    RESIZE_DIRECTION_VERTICAL,
} ResizeDirection;

extern Frame *root_frame;

Frame *new_root_frame(struct window *w);
void set_frame_size(Frame *f, int w, int h);
void equalize_frame_sizes(Frame *parent);
void add_to_frame_size(Frame *f, ResizeDirection dir, int amount);
void resize_frame(Frame *f, ResizeDirection dir, int size);
void update_window_coordinates(void);
Frame *split_frame(struct window *w, bool vertical, bool before);
Frame *split_root(bool vertical, bool before);
void remove_frame(Frame *f);
void debug_frames(void);

#endif
