#ifndef MODE_H
#define MODE_H

#include <stdbool.h>
#include "terminal/key.h"

typedef struct {
    void (*keypress)(KeyCode key);
    void (*update)(void);
} EditorModeOps;

extern const EditorModeOps normal_mode_ops;
extern const EditorModeOps command_mode_ops;
extern const EditorModeOps search_mode_ops;
extern const EditorModeOps menu_ops;

void menu_reload(char **argv, bool null_delimited);

#endif
