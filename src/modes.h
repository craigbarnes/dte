#ifndef MODES_H
#define MODES_H

#include "term.h"

typedef struct {
    void (*keypress)(int key);
    void (*update)(void);
} EditorModeOps;

extern const EditorModeOps normal_mode_ops;
extern const EditorModeOps command_mode_ops;
extern const EditorModeOps search_mode_ops;
extern const EditorModeOps git_open_ops;
extern const EditorModeOps *const modes[];

#endif
