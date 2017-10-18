#ifndef STRBUF_H
#define STRBUF_H

#include <stddef.h>
#include "macros.h"

typedef struct {
    unsigned char *buffer;
    long alloc;
    long len;
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

void strbuf_grow(StringBuffer *buf, long more) NONNULL_ARGS;
void strbuf_free(StringBuffer *buf) NONNULL_ARGS;
void strbuf_add_byte(StringBuffer *buf, unsigned char byte) NONNULL_ARGS;
long strbuf_add_ch(StringBuffer *buf, unsigned int u) NONNULL_ARGS;
long strbuf_insert_ch(StringBuffer *buf, long pos, unsigned int u) NONNULL_ARGS;
void strbuf_add_str(StringBuffer *buf, const char *str) NONNULL_ARGS;
void strbuf_add_buf(StringBuffer *buf, const char *ptr, long len) NONNULL_ARGS;
char *strbuf_steal(StringBuffer *buf, long *len) NONNULL_ARGS;
char *strbuf_steal_cstring(StringBuffer *buf) NONNULL_ARGS;
char *strbuf_cstring(StringBuffer *buf) NONNULL_ARGS;
void strbuf_make_space(StringBuffer *buf, long pos, long len) NONNULL_ARGS;
void strbuf_remove(StringBuffer *buf, long pos, long len) NONNULL_ARGS;

#endif
