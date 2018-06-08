#ifndef STRBUF_H
#define STRBUF_H

#include <stddef.h>
#include "macros.h"
#include "unicode.h"

typedef struct {
    unsigned char *buffer;
    size_t alloc;
    size_t len;
} String;

#define STRING_INIT { \
    .buffer = NULL, \
    .alloc = 0, \
    .len = 0 \
}

static inline NONNULL_ARGS void string_init(String *s)
{
    s->buffer = NULL;
    s->alloc = 0;
    s->len = 0;
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
char *string_steal(String *s, size_t *len) NONNULL_ARGS;
char *string_steal_cstring(String *s) NONNULL_ARGS;
char *string_cstring(const String *s) NONNULL_ARGS;
void string_make_space(String *s, size_t pos, size_t len) NONNULL_ARGS;
void string_remove(String *s, size_t pos, size_t len) NONNULL_ARGS;

#endif
