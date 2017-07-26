#include "file-option.h"
#include "ptr-array.h"
#include "options.h"
#include "buffer.h"
#include "regexp.h"

struct file_option {
	enum file_options_type type;
	char *type_or_pattern;
	char **strs;
};

static PTR_ARRAY(file_options);

static void set_options(char **args)
{
	int i;

	for (i = 0; args[i]; i += 2)
		set_option(args[i], args[i + 1], true, false);
}

void set_file_options(struct buffer *b)
{
	int i;

	for (i = 0; i < file_options.count; i++) {
		const struct file_option *opt = file_options.ptrs[i];

		if (opt->type == FILE_OPTIONS_FILETYPE) {
			if (streq(opt->type_or_pattern, b->options.filetype))
				set_options(opt->strs);
		} else if (b->abs_filename && regexp_match_nosub(
							opt->type_or_pattern,
							b->abs_filename,
							strlen(b->abs_filename))) {
			set_options(opt->strs);
		}
	}
}

void add_file_options(enum file_options_type type, char *to, char **strs)
{
	struct file_option *opt;
	regex_t re;

	if (type == FILE_OPTIONS_FILENAME) {
		if (!regexp_compile(&re, to, REG_NEWLINE | REG_NOSUB)) {
			free(to);
			free_strings(strs);
			return;
		}
		regfree(&re);
	}

	opt = xnew(struct file_option, 1);
	opt->type = type;
	opt->type_or_pattern = to;
	opt->strs = strs;
	ptr_array_add(&file_options, opt);
}
