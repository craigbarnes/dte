#ifndef STRBUF_H
#define STRBUF_H

#include <stdlib.h>

typedef struct {
    unsigned char *buffer;
    long alloc;
    long len;
} StringBuffer;

#define STRBUF_INIT { NULL, 0, 0 }

static inline void strbuf_init(StringBuffer *buf)
{
    buf->buffer = NULL;
    buf->alloc = 0;
    buf->len = 0;
}

static inline void strbuf_clear(StringBuffer *buf)
{
    buf->len = 0;
}

void strbuf_grow(StringBuffer *buf, long more);
void strbuf_free(StringBuffer *buf);
void strbuf_add_byte(StringBuffer *buf, unsigned char byte);
long strbuf_add_ch(StringBuffer *buf, unsigned int u);
long strbuf_insert_ch(StringBuffer *buf, long pos, unsigned int u);
void strbuf_add_str(StringBuffer *buf, const char *str);
void strbuf_add_buf(StringBuffer *buf, const char *ptr, long len);
char *strbuf_steal(StringBuffer *buf, long *len);
char *strbuf_steal_cstring(StringBuffer *buf);
char *strbuf_cstring(StringBuffer *buf);
void strbuf_make_space(StringBuffer *buf, long pos, long len);
void strbuf_remove(StringBuffer *buf, long pos, long len);

#endif
