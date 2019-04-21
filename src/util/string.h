#ifndef UTIL_STR_H
#define UTIL_STR_H

#include <stddef.h>
#include "macros.h"
#include "unicode.h"
#include "xmalloc.h"

typedef struct {
    unsigned char NONSTRING *buffer;
    size_t alloc;
    size_t len;
} String;

#define STRING_INIT { \
    .buffer = NULL, \
    .alloc = 0, \
    .len = 0 \
}

#define string_add_literal(s, l) ( \
    string_add_buf(s, l, STRLEN(l)) \
)

static inline NONNULL_ARGS void string_init(String *s)
{
    s->buffer = NULL;
    s->alloc = 0;
    s->len = 0;
}

static inline String string_new(size_t size)
{
    size = ROUND_UP(size, 16);
    return (String) {
        .buffer = size ? xmalloc(size) : NULL,
        .alloc = size,
        .len = 0
    };
}

static inline NONNULL_ARGS void string_clear(String *s)
{
    s->len = 0;
}

void string_free(String *s) NONNULL_ARGS;
void string_add_byte(String *s, unsigned char byte) NONNULL_ARGS;
size_t string_add_ch(String *s, CodePoint u) NONNULL_ARGS;
void string_add_str(String *s, const char *str) NONNULL_ARGS;
void string_add_buf(String *s, const char *ptr, size_t len) NONNULL_ARGS;
size_t string_insert_ch(String *s, size_t pos, CodePoint u) NONNULL_ARGS;
void string_sprintf(String *s, const char *fmt, ...) PRINTF(2) NONNULL_ARGS;
char *string_steal(String *s, size_t *len) NONNULL_ARGS;
char *string_steal_cstring(String *s) NONNULL_ARGS;
char *string_cstring(const String *s) NONNULL_ARGS;
void string_ensure_null_terminated(String *s) NONNULL_ARGS;
void string_make_space(String *s, size_t pos, size_t len) NONNULL_ARGS;
void string_remove(String *s, size_t pos, size_t len) NONNULL_ARGS;

#endif
