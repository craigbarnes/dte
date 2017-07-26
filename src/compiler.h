#ifndef COMPILER_H
#define COMPILER_H

#include "ptr-array.h"
#include "regexp.h"
#include "libc.h"

struct compile_error {
	char *file;
	char *msg;
	int line;
	int column;
};

struct error_format {
	bool ignore;
	signed char msg_idx;
	signed char file_idx;
	signed char line_idx;
	signed char column_idx;
	regex_t re;
};

struct compiler {
	char *name;
	struct ptr_array error_formats;
};

void add_error_fmt(const char *compiler, bool ignore, const char *format, char **desc);
struct compiler *find_compiler(const char *name);

#endif
