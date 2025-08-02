#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include <stddef.h>
#include <string.h>
#include "bit.h"
#include "macros.h"
#include "str-util.h"
#include "string-view.h"
#include "unicode.h"
#include "xmalloc.h"

typedef struct {
    char NONSTRING *buffer;
    size_t alloc;
    size_t len;
} String;

#define STRING_INIT { \
    .buffer = NULL, \
    .alloc = 0, \
    .len = 0 \
}

#define string_append_literal(s, x) string_append_buf(s, x, STRLEN(x))

void string_append_buf(String *s, const char *ptr, size_t len) NONNULL_ARG(1) NONNULL_ARG_IF_NONZERO_LENGTH(2, 3);

static inline String string_new(size_t size)
{
    size = next_multiple(size, 16);
    return (String) {
        .buffer = size ? xmalloc(size) : NULL,
        .alloc = size,
        .len = 0
    };
}

static inline String string_new_from_buf(const char *buf, size_t len)
{
    size_t alloc = next_multiple(len, 16);
    return (String) {
        .buffer = len ? memcpy(xmalloc(alloc), buf, len) : NULL,
        .alloc = alloc,
        .len = len,
    };
}

static inline void string_append_string(String *s1, const String *s2)
{
    string_append_buf(s1, s2->buffer, s2->len);
}

static inline void string_append_cstring(String *s, const char *cstr)
{
    string_append_buf(s, cstr, strlen(cstr));
}

static inline void string_append_strview(String *s, const StringView *sv)
{
    string_append_buf(s, sv->data, sv->length);
}

static inline void string_replace_byte(String *s, char byte, char rep)
{
    if (s->len) {
        strn_replace_byte(s->buffer, s->len, byte, rep);
    }
}

static inline StringView strview_from_string(const String *s)
{
    return string_view(s->buffer, s->len);
}

static inline size_t string_clear(String *s)
{
    size_t oldlen = s->len;
    s->len = 0;
    return oldlen;
}

char *string_reserve_space(String *s, size_t more) NONNULL_ARGS_AND_RETURN;
void string_append_byte(String *s, unsigned char byte) NONNULL_ARGS;
size_t string_append_codepoint(String *s, CodePoint u) NONNULL_ARGS;
size_t string_insert_codepoint(String *s, size_t pos, CodePoint u) NONNULL_ARGS;
void string_insert_buf(String *s, size_t pos, const char *buf, size_t len) NONNULL_ARG(1) NONNULL_ARG_IF_NONZERO_LENGTH(3, 4);
void string_append_memset(String *s, unsigned char byte, size_t len) NONNULL_ARGS;
void string_sprintf(String *s, const char *fmt, ...) PRINTF(2) NONNULL_ARGS;
char *string_steal_cstring(String *s) NONNULL_ARGS_AND_RETURN WARN_UNUSED_RESULT;
char *string_clone_cstring(const String *s) XSTRDUP WARN_UNUSED_RESULT;
const char *string_borrow_cstring(String *s) NONNULL_ARGS_AND_RETURN WARN_UNUSED_RESULT;
void string_remove(String *s, size_t pos, size_t len) NONNULL_ARGS;
void string_free(String *s) NONNULL_ARGS;

#endif
