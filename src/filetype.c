#include "filetype.h"
#include "common.h"
#include "regexp.h"
#include "ptr-array.h"

/*
 * Single filetype and extension/regexp pair.
 *
 * Filetypes are not grouped by name to make it possible to order them freely.
 */
struct filetype {
	char *name;
	char *str;
	enum detect_type type;
};

static PTR_ARRAY(filetypes);

static const char *ignore[] = {
	"bak", "dpkg-dist", "dpkg-old", "new", "old", "orig", "pacnew",
	"pacorig", "pacsave", "rpmnew", "rpmsave",
};

void add_filetype(const char *name, const char *str, enum detect_type type)
{
	struct filetype *ft;
	regex_t re;

	switch (type) {
	case FT_CONTENT:
	case FT_FILENAME:
		if (!regexp_compile(&re, str, REG_NEWLINE | REG_NOSUB))
			return;
		regfree(&re);
		break;
	default:
		break;
	}

	ft = xnew(struct filetype, 1);
	ft->name = xstrdup(name);
	ft->str = xstrdup(str);
	ft->type = type;
	ptr_array_add(&filetypes, ft);
}

// file.c.old~ -> c
// file..old   -> old
// file.old    -> old
static char *get_ext(const char *filename)
{
	const char *ext = strrchr(filename, '.');
	int ext_len, i;

	if (!ext)
		return NULL;

	ext++;
	ext_len = strlen(ext);
	if (ext_len && ext[ext_len - 1] == '~')
		ext_len--;
	if (!ext_len)
		return NULL;

	for (i = 0; i < ARRAY_COUNT(ignore); i++) {
		if (!strncmp(ignore[i], ext, ext_len) && !ignore[i][ext_len]) {
			int idx = -2;
			while (ext + idx >= filename) {
				if (ext[idx] == '.') {
					int len = -idx - 2;
					if (len) {
						ext -= len + 1;
						ext_len = len;
					}
					break;
				}
				idx--;
			}
			break;
		}
	}
	return xstrslice(ext, 0, ext_len);
}

const char *find_ft(const char *filename, const char *interpreter,
	const char *first_line, unsigned int line_len)
{
	unsigned int filename_len = strlen(filename);
	char *ext = NULL;
	int i;

	if (filename)
		ext = get_ext(filename);
	for (i = 0; i < filetypes.count; i++) {
		const struct filetype *ft = filetypes.ptrs[i];

		switch (ft->type) {
		case FT_EXTENSION:
			if (!ext || !streq(ext, ft->str))
				continue;
			break;
		case FT_FILENAME:
			if (!filename || !regexp_match_nosub(ft->str, filename, filename_len))
				continue;
			break;
		case FT_CONTENT:
			if (!first_line || !regexp_match_nosub(ft->str, first_line, line_len))
				continue;
			break;
		case FT_INTERPRETER:
			if (!interpreter || !streq(interpreter, ft->str))
				continue;
			break;
		}
		free(ext);
		return ft->name;
	}
	free(ext);
	return NULL;
}

bool is_ft(const char *name)
{
	int i;

	for (i = 0; i < filetypes.count; i++) {
		const struct filetype *ft = filetypes.ptrs[i];
		if (streq(ft->name, name))
			return true;
	}
	return false;
}
