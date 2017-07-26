#include "common.h"
#include "editor.h"

const char hex_tab[16] = "0123456789abcdef";
bool term_utf8;

long count_nl(const char *buf, long size)
{
	const char *end = buf + size;
	long nl = 0;

	while (buf < end) {
		buf = memchr(buf, '\n', end - buf);
		if (!buf)
			break;
		buf++;
		nl++;
	}
	return nl;
}

int count_strings(char **strings)
{
	int count;

	for (count = 0; strings[count]; count++)
		;
	return count;
}

void free_strings(char **strings)
{
	int i;
	for (i = 0; strings[i]; i++)
		free(strings[i]);
	free(strings);
}

int number_width(long n)
{
	int width = 0;

	if (n < 0) {
		n *= -1;
		width++;
	}
	do {
		n /= 10;
		width++;
	} while (n);
	return width;
}

bool buf_parse_long(const char *str, int size, int *posp, long *valp)
{
	int pos = *posp;
	int sign = 1;
	int count = 0;
	long val = 0;

	if (pos < size && str[pos] == '-') {
		sign = -1;
		pos++;
	}
	while (pos < size && isdigit(str[pos])) {
		long old = val;

		val *= 10;
		val += str[pos++] - '0';
		count++;
		if (val < old) {
			// overflow
			return false;
		}
	}
	if (count == 0)
		return false;
	*posp = pos;
	*valp = sign * val;
	return true;
}

bool parse_long(const char **strp, long *valp)
{
	const char *str = *strp;
	int size = strlen(str);
	int pos = 0;

	if (buf_parse_long(str, size, &pos, valp)) {
		*strp = str + pos;
		return true;
	}
	return false;
}

bool str_to_long(const char *str, long *valp)
{
	return parse_long(&str, valp) && *str == 0;
}

bool str_to_int(const char *str, int *valp)
{
	long val;

	if (!str_to_long(str, &val) || val < INT_MIN || val > INT_MAX)
		return false;
	*valp = val;
	return true;
}

char *xvsprintf(const char *format, va_list ap)
{
	char buf[4096];
	vsnprintf(buf, sizeof(buf), format, ap);
	return xstrdup(buf);
}

char *xsprintf(const char *format, ...)
{
	va_list ap;
	char *str;
	va_start(ap, format);
	str = xvsprintf(format, ap);
	va_end(ap);
	return str;
}

ssize_t xread(int fd, void *buf, size_t count)
{
	char *b = buf;
	size_t pos = 0;

	do {
		int rc;

		rc = read(fd, b + pos, count - pos);
		if (rc == -1) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		if (rc == 0) {
			/* eof */
			break;
		}
		pos += rc;
	} while (count - pos > 0);
	return pos;
}

ssize_t xwrite(int fd, const void *buf, size_t count)
{
	const char *b = buf;
	size_t count_save = count;

	do {
		int rc;

		rc = write(fd, b, count);
		if (rc == -1) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		b += rc;
		count -= rc;
	} while (count > 0);
	return count_save;
}

// Returns size of file or -1 on error.
// For empty file *bufp is NULL otherwise *bufp is NUL-terminated.
ssize_t read_file(const char *filename, char **bufp)
{
	struct stat st;
	return stat_read_file(filename, bufp, &st);
}

long stat_read_file(const char *filename, char **bufp, struct stat *st)
{
	int fd = open(filename, O_RDONLY);
	char *buf;
	long r;

	*bufp = NULL;
	if (fd == -1)
		return -1;
	if (fstat(fd, st) == -1) {
		close(fd);
		return -1;
	}
	buf = xnew(char, st->st_size + 1);
	r = xread(fd, buf, st->st_size);
	close(fd);
	if (r > 0) {
		buf[r] = 0;
		*bufp = buf;
	} else {
		free(buf);
	}
	return r;
}

char *buf_next_line(char *buf, ssize_t *posp, ssize_t size)
{
	ssize_t pos = *posp;
	ssize_t avail = size - pos;
	char *line = buf + pos;
	char *nl = memchr(line, '\n', avail);
	if (nl) {
		*nl = 0;
		*posp += nl - line + 1;
	} else {
		line[avail] = 0;
		*posp += avail;
	}
	return line;
}

void bug(const char *function, const char *fmt, ...)
{
	va_list ap;

	if (!child_controls_terminal && editor_status != EDITOR_INITIALIZING)
		ui_end();

	fprintf(stderr, "\n *** BUG *** %s: ", function);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	// for core dump
	abort();
}

void debug_print(const char *function, const char *fmt, ...)
{
	static int fd = -1;
	char buf[4096];
	int pos;
	va_list ap;

	if (fd < 0) {
		char *filename = editor_file("debug.log");
		fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
		free(filename);
		BUG_ON(fd < 0);

		// don't leak file descriptor to parent processes
		fcntl(fd, F_SETFD, FD_CLOEXEC);
	}

	snprintf(buf, sizeof(buf), "%s: ", function);
	pos = strlen(buf);
	va_start(ap, fmt);
	vsnprintf(buf + pos, sizeof(buf) - pos, fmt, ap);
	va_end(ap);
	xwrite(fd, buf, strlen(buf));
}
