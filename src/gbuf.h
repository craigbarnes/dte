#ifndef GBUF_H
#define GBUF_H

#include <stdlib.h>

struct gbuf {
	unsigned char *buffer;
	long alloc;
	long len;
};

#define GBUF_INIT { NULL, 0, 0 }
#define GBUF(name) struct gbuf name = GBUF_INIT

static inline void gbuf_init(struct gbuf *buf)
{
	buf->buffer = NULL;
	buf->alloc = 0;
	buf->len = 0;
}

static inline void gbuf_clear(struct gbuf *buf)
{
	buf->len = 0;
}

void gbuf_grow(struct gbuf *buf, long more);
void gbuf_free(struct gbuf *buf);
void gbuf_add_byte(struct gbuf *buf, unsigned char byte);
long gbuf_add_ch(struct gbuf *buf, unsigned int u);
long gbuf_insert_ch(struct gbuf *buf, long pos, unsigned int u);
void gbuf_add_str(struct gbuf *buf, const char *str);
void gbuf_add_buf(struct gbuf *buf, const char *ptr, long len);
char *gbuf_steal(struct gbuf *buf, long *len);
char *gbuf_steal_cstring(struct gbuf *buf);
char *gbuf_cstring(struct gbuf *buf);
void gbuf_make_space(struct gbuf *buf, long pos, long len);
void gbuf_remove(struct gbuf *buf, long pos, long len);

#endif
