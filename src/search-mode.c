#include "modes.h"
#include "cmdline.h"
#include "history.h"
#include "editor.h"
#include "search.h"
#include "options.h"

static void search_mode_keypress(int key)
{
	switch (key) {
	case KEY_ENTER:
		if (cmdline.buf.len > 0) {
			char *str = gbuf_cstring(&cmdline.buf);
			search_set_regexp(str);
			search_next();
			history_add(&search_history, str, search_history_size);
			free(str);
		} else {
			search_next();
		}
		cmdline_clear(&cmdline);
		set_input_mode(INPUT_NORMAL);
		break;
	case MOD_META | 'c':
		options.case_sensitive_search = (options.case_sensitive_search + 1) % 3;
		break;
	case MOD_META | 'r':
		search_set_direction(current_search_direction() ^ 1);
		break;
	default:
		switch (cmdline_handle_key(&cmdline, &search_history, key)) {
		case CMDLINE_UNKNOWN_KEY:
			break;
		case CMDLINE_KEY_HANDLED:
			break;
		case CMDLINE_CANCEL:
			set_input_mode(INPUT_NORMAL);
			break;
		}
	}
}

const struct editor_mode_ops search_mode_ops = {
	.keypress = search_mode_keypress,
	.update = normal_update,
};
