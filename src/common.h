#ifndef COMMON_H
#define COMMON_H

#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "libc.h"
#include "ascii.h"
#include "macros.h"
#include "xmalloc.h"

#define memzero(ptr) memset((ptr), 0, sizeof(*(ptr)))

#if DEBUG <= 0
  static inline void BUG(const char* UNUSED(fmt), ...) {}
#else
  #define BUG(...) bug(__FILE__, __LINE__, __func__, __VA_ARGS__)
#endif

#if DEBUG <= 1
  static inline void d_print(const char* UNUSED(fmt), ...) {}
#else
  #define d_print(...) debug_print(__func__, __VA_ARGS__)
#endif

#define STRINGIFY(a) #a

#define BUG_ON(a) \
    do { \
        if (unlikely(a)) { \
            BUG("%s", STRINGIFY(a)); \
        } \
    } while (0)

static inline size_t CONST_FN ROUND_UP(size_t x, size_t r)
{
    r--;
    return (x + r) & ~r;
}

static inline NONNULL_ARGS bool streq(const char *a, const char *b)
{
    return !strcmp(a, b);
}

static inline bool xstreq(const char *a, const char *b)
{
    if (a == b) {
        return true;
    } else if (a == NULL) {
        return false;
    } else if (b == NULL) {
        return false;
    }
    return streq(a, b);
}

static inline NONNULL_ARGS bool str_has_prefix (
    const char *str,
    const char *prefix
) {
    return !strncmp(str, prefix, strlen(prefix));
}

static inline NONNULL_ARGS bool str_has_suffix (
    const char *str,
    const char *suffix
) {
    size_t l1 = strlen(str);
    size_t l2 = strlen(suffix);

    if (l2 > l1) {
        return false;
    }
    return memcmp(str + l1 - l2, suffix, l2) == 0;
}

size_t count_nl(const char *buf, size_t size);
size_t count_strings(char **strings);
void free_strings(char **strings);
int number_width(long n) CONST_FN;
bool buf_parse_long(const char *str, size_t size, size_t *posp, long *valp);
bool parse_long(const char **strp, long *valp);
bool str_to_long(const char *str, long *valp);
bool str_to_int(const char *str, int *valp);
char *xvsprintf(const char *format, va_list ap);
char *xsprintf(const char *format, ...) FORMAT(1);
ssize_t xread(int fd, void *buf, size_t count);
ssize_t xwrite(int fd, const void *buf, size_t count);
ssize_t read_file(const char *filename, char **bufp);
ssize_t stat_read_file(const char *filename, char **bufp, struct stat *st);
char *buf_next_line(char *buf, ssize_t *posp, ssize_t size);
void term_cleanup(void);

NORETURN void bug (
    const char *file,
    int line,
    const char *funct,
    const char *fmt,
    ...
) FORMAT(4);

void debug_print(const char *function, const char *fmt, ...) FORMAT(2);

#endif
