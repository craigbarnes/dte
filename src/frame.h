#ifndef FRAME_H
#define FRAME_H

#include "ptr-array.h"
#include "libc.h"

struct frame {
	struct frame *parent;

	// every frame contains either one window or multiple subframes
	struct ptr_array frames;
	struct window *window;

	// width and height
	int w, h;

	bool vertical;
	bool equal_size;
};

enum resize_direction {
	RESIZE_DIRECTION_AUTO,
	RESIZE_DIRECTION_HORIZONTAL,
	RESIZE_DIRECTION_VERTICAL,
};

extern struct frame *root_frame;

struct frame *new_root_frame(struct window *w);
void set_frame_size(struct frame *f, int w, int h);
void equalize_frame_sizes(struct frame *parent);
void add_to_frame_size(struct frame *f, enum resize_direction dir, int amount);
void resize_frame(struct frame *f, enum resize_direction dir, int size);
void update_window_coordinates(void);
struct frame *split_frame(struct window *w, bool vertical, bool before);
struct frame *split_root(bool vertical, bool before);
void remove_frame(struct frame *f);
void debug_frames(void);

#endif
