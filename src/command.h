#ifndef COMMAND_H
#define COMMAND_H

#include "ptr-array.h"
#include "error.h"
#include "libc.h"

struct command {
	const char *name;
	const char *flags;
	signed char min_args;
	signed char max_args;
	void (*cmd)(const char *, char **);
};

/* parse-command.c */
char *parse_command_arg(const char *cmd, bool tilde);
int find_end(const char *cmd, int pos, struct error **err);
bool parse_commands(struct ptr_array *array, const char *cmd, struct error **err);
char **copy_string_array(char **src, int count);

/* run.c */
extern const struct command *current_command;

const struct command *find_command(const struct command *cmds, const char *name);
void run_commands(const struct command *cmds, const struct ptr_array *array);
void handle_command(const struct command *cmds, const char *cmd);

/* commands.c */
extern const struct command commands[];

#endif
