#include "xmalloc.h"
#include "common.h"

static void NORETURN malloc_fail(unsigned long size)
{
	fprintf(stderr, "out of memory: %lu\n", size);
	abort();
}

void *xmalloc(size_t size)
{
	void *ptr = malloc(size);

	if (unlikely(ptr == NULL))
		malloc_fail(size);
	return ptr;
}

void *xcalloc(size_t size)
{
	void *ptr = calloc(1, size);

	if (unlikely(ptr == NULL))
		malloc_fail(size);
	return ptr;
}

void *xrealloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);
	if (unlikely(ptr == NULL))
		malloc_fail(size);
	return ptr;
}

char *xstrdup(const char *str)
{
	char *s = strdup(str);

	if (unlikely(s == NULL))
		malloc_fail(strlen(str));
	return s;
}

char *xstrcut(const char *str, size_t size)
{
	char *s = xmalloc(size + 1);
	memcpy(s, str, size);
	s[size] = 0;
	return s;
}

void *xmemdup(const void *ptr, size_t size)
{
	void *buf = xmalloc(size);
	memcpy(buf, ptr, size);
	return buf;
}
