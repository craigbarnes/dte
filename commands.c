#include "commands.h"
#include "window.h"
#include "term.h"
#include "search.h"
#include "cmdline.h"
#include "history.h"
#include "spawn.h"

#define MAX_KEYS 4

struct binding {
	struct list_head node;
	char *command;
	enum term_key_type types[MAX_KEYS];
	unsigned int keys[MAX_KEYS];
	int nr_keys;
};

int nr_pressed_keys;

static enum term_key_type pressed_types[MAX_KEYS];
static unsigned int pressed_keys[MAX_KEYS];

static LIST_HEAD(bindings);
static const char *special_names[NR_SKEYS] = {
	"backspace",
	"insert",
	"delete",
	"home",
	"end",
	"pgup",
	"pgdown",
	"left",
	"right",
	"up",
	"down",
	"f1",
	"f2",
	"f3",
	"f4",
	"f5",
	"f6",
	"f7",
	"f8",
	"f9",
	"f10",
	"f11",
	"f12",
};

static int parse_key(enum term_key_type *type, unsigned int *key, const char *str)
{
	int i, len = strlen(str);
	char ch;

	if (len == 1) {
		*type = KEY_NORMAL;
		*key = str[0];
		return 1;
	}
	ch = toupper(str[1]);
	if (str[0] == '^' && ch >= 0x40 && ch < 0x60 && len == 2) {
		*type = KEY_NORMAL;
		*key = ch - 0x40;
		return 1;
	}
	if (str[0] == 'M' && str[1] == '-' && parse_key(type, key, str + 2)) {
		*type = KEY_META;
		return 1;
	}
	for (i = 0; i < NR_SKEYS; i++) {
		if (!strcasecmp(str, special_names[i])) {
			*type = KEY_SPECIAL;
			*key = i;
			return 1;
		}
	}
	return 0;
}

static const char *parse_flags(char ***argsp, const char *flags)
{
	static char parsed[16];
	int i, j, count = 0;
	char **args = *argsp;

	parsed[0] = 0;
	for (i = 0; args[i]; i++) {
		const char *arg = args[i];

		if (!strcmp(arg, "--")) {
			i++;
			break;
		}
		if (arg[0] != '-' || !arg[1])
			break;
		for (j = 1; arg[j]; j++) {
			char flag = arg[j];
			if (!strchr(flags, flag)) {
				error_msg("Invalid option -%c", flag);
				return NULL;
			}
			if (!strchr(parsed, flag)) {
				parsed[count++] = flag;
				parsed[count] = 0;
			}
		}
	}
	*argsp = args + i;
	return parsed;
}

static const char *parse_args(char ***argsp, const char *flags, int min, int max)
{
	const char *pf = parse_flags(argsp, flags);

	if (pf) {
		char **args = *argsp;
		int argc = 0;

		while (args[argc])
			argc++;
		if (argc < min) {
			error_msg("Not enough arguments");
			return NULL;
		}
		if (max >= 0 && argc > max) {
			error_msg("Too many arguments");
			return NULL;
		}
	}
	d_print("flags: %s\n", pf);
	return pf;
}

#define ARGC(min, max) \
	int argc = 0; \
	while (args[argc]) \
		argc++; \
	if (argc < min) { \
		d_print("not enough arguments\n"); \
		return; \
	} \
	if (max >= 0 && argc > max) { \
		d_print("too many arguments\n"); \
		return; \
	}

static void cmd_backspace(char **args)
{
	backspace();
}

static void cmd_bind(char **args)
{
	struct binding *b;
	char *keys;
	int count = 0, i = 0;

	ARGC(2, 2);

	b = xnew(struct binding, 1);
	b->command = xstrdup(args[1]);

	keys = args[0];
	while (keys[i]) {
		int start = i;

		if (count >= MAX_KEYS)
			goto error;

		i++;
		while (keys[i] && keys[i] != ',')
			i++;
		if (keys[i] == ',')
			keys[i++] = 0;
		if (!parse_key(&b->types[count], &b->keys[count], keys + start))
			goto error;
		count++;
	}
	b->nr_keys = count;
	if (!count)
		goto error;

	list_add_before(&b->node, &bindings);
	return;
error:
	free(b->command);
	free(b);
}

static void cmd_bof(char **args)
{
	move_bof();
}

static void cmd_bol(char **args)
{
	move_bol();
}

static void cmd_cancel(char **args)
{
	select_end();
}

static void cmd_center_cursor(char **args)
{
	const char *pf = parse_args(&args, "", 0, 0);

	if (!pf)
		return;
	center_cursor();
}

static void cmd_close(char **args)
{
	const char *pf = parse_args(&args, "f", 0, 0);
	struct window *w;
	struct view *v;
	int count = 0;

	if (!pf)
		return;

	list_for_each_entry(w, &windows, node) {
		list_for_each_entry(v, &w->views, node) {
			if (v->buffer == view->buffer)
				count++;
		}
	}
	if (buffer_modified(buffer) && count == 1 && !*pf) {
		error_msg("The buffer is modified. Save or run 'close -f' to close without saving.");
		return;
	}
	if (count == 1)
		free_buffer(view->buffer);
	remove_view();
}

static void cmd_command(char **args)
{
	input_mode = INPUT_COMMAND;
	update_flags |= UPDATE_STATUS_LINE;

	if (args[0])
		cmdline_set_text(args[0]);
}

static void cmd_copy(char **args)
{
	unsigned int len;

	undo_merge = UNDO_MERGE_NONE;
	len = prepare_selection();
	copy(len, view->sel_is_lines);
	select_end();
}

static void cmd_cut(char **args)
{
	unsigned int len;

	undo_merge = UNDO_MERGE_NONE;
	len = prepare_selection();
	cut(len, view->sel_is_lines);
	select_end();
}

static void cmd_delete(char **args)
{
	delete_ch();
}

static void cmd_delete_bol(char **args)
{
	struct block_iter bi = view->cursor;
	unsigned int len = block_iter_bol(&bi);

	SET_CURSOR(bi);

	undo_merge = UNDO_MERGE_NONE;
	delete(len, 1);
}

static void cmd_delete_eol(char **args)
{
	struct block_iter bi = view->cursor;
	unsigned int len = count_bytes_eol(&bi) - 1;

	undo_merge = UNDO_MERGE_NONE;
	delete(len, 0);
}

static void cmd_down(char **args)
{
	move_down(1);
}

static void cmd_eof(char **args)
{
	move_eof();
}

static void cmd_eol(char **args)
{
	move_eol();
}

static void cmd_error(char **args)
{
	const char *pf = parse_args(&args, "np", 0, 1);
	char dir = 0;

	if (!pf)
		return;

	while (*pf) {
		switch (*pf) {
		case 'n':
		case 'p':
			dir = *pf;
			break;
		}
		pf++;
	}

	if (!cerr.count) {
		info_msg("No errors");
		return;
	}
	if (dir == 'n') {
		if (cerr.pos == cerr.count - 1) {
			info_msg("No more errors");
			return;
		}
		cerr.pos++;
	} else if (dir == 'p') {
		if (cerr.pos <= 0) {
			info_msg("No previous errors");
			return;
		}
		cerr.pos--;
	} else if (*args) {
		int num = atoi(*args);
		if (num < 1 || num > cerr.count) {
			info_msg("There are %d errors", cerr.count);
			return;
		}
		cerr.pos = num - 1;
	} else {
		// default is current error
		if (cerr.pos < 0)
			cerr.pos = 0;
	}
	show_compile_error();
}

static char *shell_unescape(const char *str)
{
	GBUF(buf);
	int i = 0;

	while (str[i]) {
		char ch = str[i++];
		if (ch == '\\' && str[i]) {
			ch = str[i++];
			switch (ch) {
			case 'n':
				ch = '\n';
				break;
			case 'r':
				ch = '\r';
				break;
			}
		}
		gbuf_add_ch(&buf, ch);
	}
	return gbuf_steal(&buf);
}

static void cmd_insert(char **args)
{
	const char *pf = parse_args(&args, "ekm", 1, 1);
	const char *str = args[0];
	char *buf = NULL;

	if (!pf)
		return;

	if (strchr(pf, 'e')) {
		buf = shell_unescape(str);
		str = buf;
	}

	if (view->sel.blk)
		delete_ch();
	undo_merge = UNDO_MERGE_NONE;
	if (strchr(pf, 'k')) {
		int i;
		for (i = 0; str[i]; i++)
			insert_ch(str[i]);
	} else {
		int len = strlen(str);

		insert(str, len);
		if (strchr(pf, 'm'))
			move_offset(buffer_offset() + len);
	}
	free(buf);
}

static void cmd_join(char **args)
{
	join_lines();
}

static void cmd_left(char **args)
{
	move_left(1);
}

static void cmd_line(char **args)
{
	int line;

	ARGC(1, 1);
	line = atoi(args[0]);
	if (line > 0)
		move_to_line(line);
}

static void cmd_next(char **args)
{
	next_buffer();
}

static void cmd_open(char **args)
{
	struct view *v;

	ARGC(0, 1);
	v = open_buffer(args[0]);
	if (v)
		set_view(v);
}

static void cmd_paste(char **args)
{
	paste();
}

static void cmd_pgdown(char **args)
{
	move_down(window->h - 1);
}

static void cmd_pgup(char **args)
{
	move_up(window->h - 1);
}

static void cmd_pop(char **args)
{
	pop_location();
}

static void cmd_prev(char **args)
{
	prev_buffer();
}

static void cmd_push(char **args)
{
	push_location();
}

static void cmd_quit(char **args)
{
	const char *pf = parse_args(&args, "f", 0, 0);
	struct window *w;
	struct view *v;

	if (!pf)
		return;
	if (pf[0]) {
		running = 0;
		return;
	}
	list_for_each_entry(w, &windows, node) {
		list_for_each_entry(v, &w->views, node) {
			if (buffer_modified(v->buffer)) {
				error_msg("Save modified files or run 'quit -f' to quit without saving.");
				return;
			}
		}
	}
	running = 0;
}

static void cmd_redo(char **args)
{
	redo();
}

static void cmd_replace(char **args)
{
	const char *pf = parse_args(&args, "bcgi", 2, 2);
	unsigned int flags = 0;
	int i;

	if (!pf)
		return;

	for (i = 0; pf[i]; i++) {
		switch (pf[i]) {
		case 'b':
			flags |= REPLACE_BASIC;
			break;
		case 'c':
			flags |= REPLACE_CONFIRM;
			break;
		case 'g':
			flags |= REPLACE_GLOBAL;
			break;
		case 'i':
			flags |= REPLACE_IGNORE_CASE;
			break;
		}
	}
	reg_replace(args[0], args[1], flags);
}

static void cmd_right(char **args)
{
	move_right(1);
}

static void cmd_run(char **args)
{
	const char *pf = parse_args(&args, "cejps", 1, -1);
	unsigned int flags = 0;
	int quoted = 0;

	if (!pf)
		return;

	while (*pf) {
		switch (*pf) {
		case 'c':
			quoted = 1;
			break;
		case 'e':
			flags |= SPAWN_COLLECT_ERRORS;
			break;
		case 'j':
			flags |= SPAWN_COLLECT_ERRORS | SPAWN_JUMP_TO_ERROR;
			break;
		case 'p':
			flags |= SPAWN_PROMPT;
			break;
		case 's':
			flags |= SPAWN_REDIRECT_STDOUT | SPAWN_REDIRECT_STDERR;
			break;
		}
		pf++;
	}
	if (quoted) {
		struct parsed_command pc;
		char cmd[8192];
		char *word;

		if (args[1]) {
			error_msg("Too many arguments");
			return;
		}

		word = get_word_under_cursor();
		if (!word) {
			return;
		}

		snprintf(cmd, sizeof(cmd), args[0], word);
		free(word);
		if (parse_commands(&pc, cmd, 0)) {
			free_commands(&pc);
			return;
		}
		spawn(pc.argv, flags);
		free_commands(&pc);
	} else {
		spawn(args, flags);
	}
}

static void cmd_save(char **args)
{
	ARGC(0, 1);

	if (args[0]) {
		char *absolute = path_absolute(args[0]);

		if (!absolute) {
			error_msg("Failed to make absolute path.");
			return;
		}
		free(buffer->filename);
		free(buffer->abs_filename);
		buffer->filename = xstrdup(args[0]);
		buffer->abs_filename = absolute;
		buffer->ro = 0;
	}
	save_buffer();
}

static void do_search_next(char **args, enum search_direction dir)
{
	const char *pf = parse_args(&args, "w", 0, 0);

	if (!pf)
		return;

	if (*pf) {
		char *pattern, *word = get_word_under_cursor();

		if (!word) {
			return;
		}
		pattern = xnew(char, strlen(word) + 5);
		sprintf(pattern, "\\<%s\\>", word);
		search_init(dir);
		search(pattern, REG_EXTENDED | REG_NEWLINE);
		history_add(&search_history, pattern);
		free(word);
		free(pattern);
	} else {
		input_mode = INPUT_SEARCH;
		search_init(dir);
		update_flags |= UPDATE_STATUS_LINE;
	}
}

static void cmd_search_bwd(char **args)
{
	do_search_next(args, SEARCH_BWD);
}

static void cmd_search_fwd(char **args)
{
	do_search_next(args, SEARCH_FWD);
}

static void cmd_search_next(char **args)
{
	search_next();
}

static void cmd_search_prev(char **args)
{
	search_prev();
}

static void cmd_select(char **args)
{
	int is_lines = 0;
	int i = 0;

	ARGC(0, 1);
	while (i < argc) {
		const char *arg = args[i++];

		if (!strcmp(arg, "--lines")) {
			is_lines = 1;
			continue;
		}
	}
	select_start(is_lines);
}

static void cmd_tag(char **args)
{
	ARGC(0, 1);

	if (args[0]) {
		goto_tag(args[0]);
	} else {
		char *word = get_word_under_cursor();
		if (!word) {
			return;
		}
		goto_tag(word);
		free(word);
	}
}

static void cmd_undo(char **args)
{
	undo();
}

static void cmd_up(char **args)
{
	move_up(1);
}

const struct command commands[] = {
	{ "backspace", NULL, cmd_backspace },
	{ "bind", NULL, cmd_bind },
	{ "bof", NULL, cmd_bof },
	{ "bol", NULL, cmd_bol },
	{ "cancel", NULL, cmd_cancel },
	{ "center-cursor", NULL, cmd_center_cursor },
	{ "close", "cl", cmd_close },
	{ "command", NULL, cmd_command },
	{ "copy", NULL, cmd_copy },
	{ "cut", NULL, cmd_cut },
	{ "delete", NULL, cmd_delete },
	{ "delete-bol", NULL, cmd_delete_bol },
	{ "delete-eol", NULL, cmd_delete_eol },
	{ "down", NULL, cmd_down },
	{ "eof", NULL, cmd_eof },
	{ "eol", NULL, cmd_eol },
	{ "error", NULL, cmd_error },
	{ "insert", NULL, cmd_insert },
	{ "join", NULL, cmd_join },
	{ "left", NULL, cmd_left },
	{ "line", NULL, cmd_line },
	{ "next", NULL, cmd_next },
	{ "open", "o", cmd_open },
	{ "paste", NULL, cmd_paste },
	{ "pgdown", NULL, cmd_pgdown },
	{ "pgup", NULL, cmd_pgup },
	{ "pop", NULL, cmd_pop },
	{ "prev", NULL, cmd_prev },
	{ "push", NULL, cmd_push },
	{ "quit", "q", cmd_quit },
	{ "redo", NULL, cmd_redo },
	{ "replace", "r", cmd_replace },
	{ "right", NULL, cmd_right },
	{ "run", NULL, cmd_run },
	{ "save", "s", cmd_save },
	{ "search-bwd", NULL, cmd_search_bwd },
	{ "search-fwd", NULL, cmd_search_fwd },
	{ "search-next", NULL, cmd_search_next },
	{ "search-prev", NULL, cmd_search_prev },
	{ "select", NULL, cmd_select },
	{ "tag", "t", cmd_tag },
	{ "undo", NULL, cmd_undo },
	{ "up", NULL, cmd_up },
	{ NULL, NULL, NULL }
};

void handle_binding(enum term_key_type type, unsigned int key)
{
	struct binding *b;

	pressed_types[nr_pressed_keys] = type;
	pressed_keys[nr_pressed_keys] = key;
	nr_pressed_keys++;

	list_for_each_entry(b, &bindings, node) {
		if (b->nr_keys < nr_pressed_keys)
			continue;

		if (memcmp(b->keys, pressed_keys, nr_pressed_keys * sizeof(pressed_keys[0])))
			continue;
		if (memcmp(b->types, pressed_types, nr_pressed_keys * sizeof(pressed_types[0])))
			continue;

		if (b->nr_keys == nr_pressed_keys) {
			undo_merge = UNDO_MERGE_NONE;
			handle_command(b->command);
			nr_pressed_keys = 0;
		}
		return;
	}
	nr_pressed_keys = 0;
}

const struct command *find_command(const char *name)
{
	int i;

	for (i = 0; commands[i].name; i++) {
		const struct command *cmd = &commands[i];

		if (!strcmp(name, cmd->name))
			return cmd;
		if (cmd->short_name && !strcmp(name, cmd->short_name))
			return cmd;
	}
	return NULL;
}

static void run_command(char **av)
{
	const struct command *cmd = find_command(av[0]);
	if (cmd)
		cmd->cmd(av + 1);
	else
		error_msg("No such command: %s", av[0]);
}

void handle_command(const char *cmd)
{
	struct parsed_command pc;
	int s, e;

	if (parse_commands(&pc, cmd, 0)) {
		free_commands(&pc);
		return;
	}

	s = 0;
	while (s < pc.count) {
		e = s;
		while (e < pc.count && pc.argv[e])
			e++;

		if (e > s)
			run_command(pc.argv + s);

		s = e + 1;
	}

	free_commands(&pc);
}
