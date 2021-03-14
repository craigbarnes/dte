#ifndef UTIL_STR_H
#define UTIL_STR_H

#include <stddef.h>
#include "macros.h"
#include "string-view.h"
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

#define string_append_literal(s, x) string_append_buf(s, x, STRLEN(x))

static inline String string_new(size_t size)
{
    size = round_size_to_next_multiple(size, 16);
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

static inline StringView strview_from_string(const String *s)
{
    return string_view(s->buffer, s->len);
}

void string_free(String *s) NONNULL_ARGS;
void string_append_byte(String *s, unsigned char byte) NONNULL_ARGS;
size_t string_append_codepoint(String *s, CodePoint u) NONNULL_ARGS;
void string_append_cstring(String *s, const char *cstr) NONNULL_ARGS;
void string_append_string(String *s1, const String *s2) NONNULL_ARGS;
void string_append_strview(String *s, const StringView *sv) NONNULL_ARGS;
void string_append_buf(String *s, const char *ptr, size_t len) NONNULL_ARG(1);
size_t string_insert_ch(String *s, size_t pos, CodePoint u) NONNULL_ARGS;
void string_insert_buf(String *s, size_t pos, const char *buf, size_t len) NONNULL_ARG(1);
void string_sprintf(String *s, const char *fmt, ...) PRINTF(2) NONNULL_ARGS;
char *string_steal_cstring(String *s) NONNULL_ARGS_AND_RETURN;
char *string_clone_cstring(const String *s) XSTRDUP;
const char *string_borrow_cstring(String *s) NONNULL_ARGS_AND_RETURN;
void string_remove(String *s, size_t pos, size_t len) NONNULL_ARGS;

#endif
