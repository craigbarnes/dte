#include "modes.h"
#include "cmdline.h"
#include "history.h"
#include "editor.h"
#include "command.h"
#include "completion.h"
#include "error.h"

static void command_line_enter(void)
{
	PTR_ARRAY(array);
	char *str = gbuf_cstring(&cmdline.buf);
	struct error *err = NULL;
	bool ok;

	reset_completion();
	set_input_mode(INPUT_NORMAL);
	ok = parse_commands(&array, str, &err);

	/* Need to do this before executing the command because
	 * "command" can modify contents of command line.
	 */
	history_add(&command_history, str, command_history_size);
	free(str);
	cmdline_clear(&cmdline);

	if (ok) {
		run_commands(commands, &array);
	} else {
		error_msg("Parsing command: %s", err->msg);
		error_free(err);
	}
	ptr_array_free(&array);
}

static void command_mode_keypress(int key)
{
	switch (key) {
	case KEY_ENTER:
		command_line_enter();
		break;
	case '\t':
		complete_command();
		break;
	default:
		switch (cmdline_handle_key(&cmdline, &command_history, key)) {
		case CMDLINE_UNKNOWN_KEY:
			break;
		case CMDLINE_KEY_HANDLED:
			reset_completion();
			break;
		case CMDLINE_CANCEL:
			set_input_mode(INPUT_NORMAL);
			break;
		}
	}
}

const struct editor_mode_ops command_mode_ops = {
	.keypress = command_mode_keypress,
	.update = normal_update,
};
