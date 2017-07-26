#ifndef COMMON_H
#define COMMON_H

#include "libc.h"
#include "ctype.h"
#include "xmalloc.h"

extern const char hex_tab[16];
extern bool term_utf8;

static inline size_t ROUND_UP(size_t x, size_t r)
{
	r--;
	return (x + r) & ~r;
}

#if DEBUG <= 0
FORMAT(1)
static inline void BUG(const char *fmt, ...)
{
}
#else
#define BUG(...) bug(__FUNCTION__, __VA_ARGS__)
#endif

#if DEBUG <= 1
FORMAT(1)
static inline void d_print(const char *fmt, ...)
{
}
#else
#define d_print(...) debug_print(__FUNCTION__, __VA_ARGS__)
#endif

#define STRINGIFY(a) #a
#define BUG_ON(a) \
	do { \
		if (unlikely(a)) \
			BUG("%s\n", STRINGIFY(a)); \
	} while (0)

static inline bool streq(const char *a, const char *b)
{
	return !strcmp(a, b);
}

static inline bool xstreq(const char *a, const char *b)
{
	if (a == b) {
		return true;
	}
	if (a == NULL) {
		return false;
	}
	if (b == NULL) {
		return false;
	}
	return streq(a, b);
}

static inline bool str_has_prefix(const char *str, const char *prefix)
{
	return !strncmp(str, prefix, strlen(prefix));
}

static inline bool str_has_suffix(const char *str, const char *suffix)
{
	int l1 = strlen(str);
	int l2 = strlen(suffix);

	if (l2 > l1) {
		return false;
	}
	return memcmp(str + l1 - l2, suffix, l2) == 0;
}

long count_nl(const char *buf, long size);
int count_strings(char **strings);
void free_strings(char **strings);
int number_width(long n);
bool buf_parse_long(const char *str, int size, int *posp, long *valp);
bool parse_long(const char **strp, long *valp);
bool str_to_long(const char *str, long *valp);
bool str_to_int(const char *str, int *valp);
char *xvsprintf(const char *format, va_list ap);
char *xsprintf(const char *format, ...) FORMAT(1);
ssize_t xread(int fd, void *buf, size_t count);
ssize_t xwrite(int fd, const void *buf, size_t count);
ssize_t read_file(const char *filename, char **bufp);
long stat_read_file(const char *filename, char **bufp, struct stat *st);
char *buf_next_line(char *buf, ssize_t *posp, ssize_t size);
void bug(const char *function, const char *fmt, ...) FORMAT(2) NORETURN;
void debug_print(const char *function, const char *fmt, ...) FORMAT(2);

#endif
