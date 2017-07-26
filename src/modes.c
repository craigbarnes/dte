#include "modes.h"

const struct editor_mode_ops * const modes[] = {
	&normal_mode_ops,
	&command_mode_ops,
	&search_mode_ops,
	&git_open_ops,
};
