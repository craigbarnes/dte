#ifndef MODES_H
#define MODES_H

#include "term.h"

struct editor_mode_ops {
	void (*keypress)(int key);
	void (*update)(void);
};

extern const struct editor_mode_ops normal_mode_ops;
extern const struct editor_mode_ops command_mode_ops;
extern const struct editor_mode_ops search_mode_ops;
extern const struct editor_mode_ops git_open_ops;
extern const struct editor_mode_ops * const modes[];

#endif
