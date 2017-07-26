#include "tag.h"
#include "ctags.h"
#include "ptr-array.h"
#include "completion.h"
#include "path.h"
#include "common.h"

static struct tag_file *current_tag_file;
static char *current_filename; // for sorting tags

static int visibility_cmp(const struct tag *a, const struct tag *b)
{
	bool a_this_file = false;
	bool b_this_file = false;

	if (!a->local && !b->local)
		return 0;

	// Is tag visibility limited to the current file?
	if (a->local)
		a_this_file = current_filename && streq(current_filename, a->filename);
	if (b->local)
		b_this_file = current_filename && streq(current_filename, b->filename);

	// Tags local to other file than current are not interesting.
	if (a->local && !a_this_file) {
		// a is not interesting
		if (b->local && !b_this_file) {
			// b is equally uninteresting
			return 0;
		}
		// b is more interesting, sort it before a
		return 1;
	}
	if (b->local && !b_this_file) {
		// b is not interesting
		return -1;
	}

	// both are NOT UNinteresting

	if (a->local && a_this_file) {
		if (b->local && b_this_file)
			return 0;
		// a is more interesting bacause it is local symbol
		return -1;
	}
	if (b->local && b_this_file) {
		// b is more interesting bacause it is local symbol
		return 1;
	}
	return 0;
}

static int kind_cmp(const struct tag *a, const struct tag *b)
{
	if (a->kind == b->kind)
		return 0;

	// Struct member (m) is not very interesting.
	if (a->kind == 'm')
		return 1;
	if (b->kind == 'm')
		return -1;

	// Global variable (v) is not very interesting.
	if (a->kind == 'v')
		return 1;
	if (b->kind == 'v')
		return -1;

	// Struct (s), union (u)
	return 0;
}

static int tag_cmp(const void *ap, const void *bp)
{
	const struct tag *a = *(const struct tag **)ap;
	const struct tag *b = *(const struct tag **)bp;
	int ret;

	ret = visibility_cmp(a, b);
	if (ret)
		return ret;

	return kind_cmp(a, b);
}

// find "tags" file from directory path and its parent directories
static int open_tag_file(char *path)
{
	const char tags[] = "tags";

	while (*path) {
		int fd, len = strlen(path);
		char *slash = strrchr(path, '/');

		if (slash != path + len - 1) {
			path[len++] = '/';
		}
		memcpy(path + len, tags, sizeof(tags));
		fd = open(path, O_RDONLY);
		if (fd >= 0) {
			return fd;
		}
		if (errno != ENOENT) {
			return -1;
		}
		*slash = 0;
	}
	errno = ENOENT;
	return -1;
}

static bool tag_file_changed(struct tag_file *tf, const char *filename, struct stat *st)
{
	if (tf->mtime != st->st_mtime) {
		return true;
	}
	return !streq(tf->filename, filename);
}

static void tag_file_free(struct tag_file *tf)
{
	free(tf->filename);
	free(tf->buf);
	free(tf);
}

struct tag_file *load_tag_file(void)
{
	struct tag_file *tf;
	struct stat st;
	char path[4096];
	char *buf;
	long size;
	int fd;

	// 5 = length of "/tags"
	if (!getcwd(path, sizeof(path) - 5)) {
		return NULL;
	}
	fd = open_tag_file(path);
	if (fd < 0) {
		return NULL;
	}
	fstat(fd, &st);
	if (current_tag_file != NULL && tag_file_changed(current_tag_file, path, &st)) {
		tag_file_free(current_tag_file);
		current_tag_file = NULL;
	}
	if (current_tag_file != NULL) {
		close(fd);
		return current_tag_file;
	}
	buf = xnew(char, st.st_size);
	size = xread(fd, buf, st.st_size);
	if (size < 0) {
		close(fd);
		return NULL;
	}
	tf = xnew0(struct tag_file, 1);
	tf->filename = xstrdup(path);
	tf->buf = buf;
	tf->size = size;
	tf->mtime = st.st_mtime;
	current_tag_file = tf;
	return current_tag_file;
}

void free_tags(struct ptr_array *tags)
{
	int i;
	for (i = 0; i < tags->count; i++) {
		struct tag *t = tags->ptrs[i];
		free_tag(t);
		free(t);
	}
	free(tags->ptrs);
	clear(tags);
}

// both parameters must be absolute and clean
static char *path_relative(const char *filename, const char *dir)
{
	int dlen = strlen(dir);

	if (!str_has_prefix(filename, dir)) {
		return NULL;
	}
	if (filename[dlen] == 0) {
		// equal strings
		return xstrdup(".");
	}
	if (filename[dlen] != '/') {
		return NULL;
	}
	return xstrdup(filename + dlen + 1);
}

void tag_file_find_tags(struct tag_file *tf, const char *filename, const char *name, struct ptr_array *tags)
{
	struct tag *t;
	size_t pos = 0;

	t = xnew(struct tag, 1);
	while (next_tag(tf, &pos, name, 1, t)) {
		ptr_array_add(tags, t);
		t = xnew(struct tag, 1);
	}
	free(t);

	if (filename == NULL) {
		current_filename = NULL;
	} else {
		char *dir = path_dirname(tf->filename);
		current_filename = path_relative(filename, dir);
		free(dir);
	}
	qsort(tags->ptrs, tags->count, sizeof(tags->ptrs[0]), tag_cmp);
	free(current_filename);
	current_filename = NULL;
}

char *tag_file_get_tag_filename(struct tag_file *tf, struct tag *t)
{
	char *dir = path_dirname(tf->filename);
	int a = strlen(dir);
	int b = strlen(t->filename);
	char *filename = xnew(char, a + b + 2);

	memcpy(filename, dir, a);
	filename[a] = '/';
	memcpy(filename + a + 1, t->filename, b + 1);
	free(dir);
	return filename;
}

void collect_tags(struct tag_file *tf, const char *prefix)
{
	struct tag t;
	size_t pos = 0;
	char *prev = NULL;

	while (next_tag(tf, &pos, prefix, 0, &t)) {
		if (!prev || !streq(prev, t.name)) {
			add_completion(t.name);
			prev = t.name;
			t.name = NULL;
		}
		free_tag(&t);
	}
}
