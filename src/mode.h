#ifndef MODE_H
#define MODE_H

#include "terminal/key.h"

typedef struct {
    void (*keypress)(KeyCode key);
    void (*update)(void);
} EditorModeOps;

extern const EditorModeOps normal_mode_ops;
extern const EditorModeOps command_mode_ops;
extern const EditorModeOps search_mode_ops;
extern const EditorModeOps git_open_ops;

void git_open_reload(void);

#endif
