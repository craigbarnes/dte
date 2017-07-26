#ifndef XMALLOC_H
#define XMALLOC_H

#include <stdlib.h>

#if defined(__GNUC__) && (__GNUC__ >= 3)
#define MALLOC	__attribute__((__malloc__))
#else
#define MALLOC
#endif

#define xnew(type, n)		(type *)xmalloc(sizeof(type) * (n))
#define xnew0(type, n)		(type *)xcalloc(sizeof(type) * (n))
#define xrenew(mem, n)		do { \
					mem = xrealloc(mem, sizeof(*mem) * (n)); \
				} while (0)

void * MALLOC xmalloc(size_t size);
void * MALLOC xcalloc(size_t size);
void * MALLOC xrealloc(void *ptr, size_t size);
char * MALLOC xstrdup(const char *str);
char * MALLOC xstrcut(const char *str, size_t size);
void * MALLOC xmemdup(const void *ptr, size_t size);

static inline char *xstrslice(const char *str, size_t pos, size_t end)
{
	return xstrcut(str + pos, end - pos);
}

#endif
