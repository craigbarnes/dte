#include "modes.h"

const EditorModeOps *const modes[] = {
    &normal_mode_ops,
    &command_mode_ops,
    &search_mode_ops,
    &git_open_ops,
};
