#ifndef SPAWN_H
#define SPAWN_H

#include "compiler.h"

/* Errors are read from stderr by default. */
#define SPAWN_READ_STDOUT	(1 << 0)

/* Redirect to /dev/null? */
#define SPAWN_QUIET		(1 << 2)

/* Press any key to continue */
#define SPAWN_PROMPT		(1 << 4)

struct filter_data {
	char *in;
	char *out;
	long in_len;
	long out_len;
};

int spawn_filter(char **argv, struct filter_data *data);
void spawn_compiler(char **args, unsigned int flags, struct compiler *c);
void spawn(char **args, int fd[3], bool prompt);

#endif
