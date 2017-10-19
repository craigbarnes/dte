#ifndef STRBUF_H
#define STRBUF_H

#include <stddef.h>
#include "macros.h"
#include "unicode.h"

typedef struct {
    unsigned char *buffer;
    size_t alloc;
    size_t len;
} StringBuffer;

#define STRBUF_INIT { \
    .buffer = NULL, \
    .alloc = 0, \
    .len = 0 \
}

static inline NONNULL_ARGS void strbuf_init(StringBuffer *buf)
{
    buf->buffer = NULL;
    buf->alloc = 0;
    buf->len = 0;
}

static inline NONNULL_ARGS void strbuf_clear(StringBuffer *buf)
{
    buf->len = 0;
}

void strbuf_grow(StringBuffer *buf, size_t more) NONNULL_ARGS;
void strbuf_free(StringBuffer *buf) NONNULL_ARGS;
void strbuf_add_byte(StringBuffer *buf, unsigned char byte) NONNULL_ARGS;
size_t strbuf_add_ch(StringBuffer *buf, CodePoint u) NONNULL_ARGS;
size_t strbuf_insert_ch(StringBuffer *buf, size_t pos, CodePoint u) NONNULL_ARGS;
void strbuf_add_str(StringBuffer *buf, const char *str) NONNULL_ARGS;
void strbuf_add_buf(StringBuffer *buf, const char *ptr, size_t len) NONNULL_ARGS;
char *strbuf_steal(StringBuffer *buf, size_t *len) NONNULL_ARGS;
char *strbuf_steal_cstring(StringBuffer *buf) NONNULL_ARGS;
char *strbuf_cstring(StringBuffer *buf) NONNULL_ARGS;
void strbuf_make_space(StringBuffer *buf, size_t pos, size_t len) NONNULL_ARGS;
void strbuf_remove(StringBuffer *buf, size_t pos, size_t len) NONNULL_ARGS;

#endif
