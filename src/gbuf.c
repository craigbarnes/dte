#include "gbuf.h"
#include "common.h"
#include "uchar.h"

void gbuf_grow(struct gbuf *buf, long more)
{
	long alloc = ROUND_UP(buf->len + more, 16);

	if (alloc > buf->alloc) {
		buf->alloc = alloc;
		buf->buffer = xrealloc(buf->buffer, buf->alloc);
	}
}

void gbuf_free(struct gbuf *buf)
{
	free(buf->buffer);
	gbuf_init(buf);
}

void gbuf_add_byte(struct gbuf *buf, unsigned char byte)
{
	gbuf_grow(buf, 1);
	buf->buffer[buf->len++] = byte;
}

long gbuf_add_ch(struct gbuf *buf, unsigned int u)
{
	unsigned int len = u_char_size(u);

	gbuf_grow(buf, len);
	u_set_char_raw(buf->buffer, &buf->len, u);
	return len;
}

long gbuf_insert_ch(struct gbuf *buf, long pos, unsigned int u)
{
	unsigned int len = u_char_size(u);

	gbuf_make_space(buf, pos, len);
	u_set_char_raw(buf->buffer, &pos, u);
	return len;
}

void gbuf_add_str(struct gbuf *buf, const char *str)
{
	gbuf_add_buf(buf, str, strlen(str));
}

void gbuf_add_buf(struct gbuf *buf, const char *ptr, long len)
{
	if (!len)
		return;
	gbuf_grow(buf, len);
	memcpy(buf->buffer + buf->len, ptr, len);
	buf->len += len;
}

char *gbuf_steal(struct gbuf *buf, long *len)
{
	char *b = buf->buffer;
	*len = buf->len;
	gbuf_init(buf);
	return b;
}

char *gbuf_steal_cstring(struct gbuf *buf)
{
	char *b;
	gbuf_add_byte(buf, 0);
	b = buf->buffer;
	gbuf_init(buf);
	return b;
}

char *gbuf_cstring(struct gbuf *buf)
{
	char *b = xnew(char, buf->len + 1);
	memcpy(b, buf->buffer, buf->len);
	b[buf->len] = 0;
	return b;
}

void gbuf_make_space(struct gbuf *buf, long pos, long len)
{
	BUG_ON(pos > buf->len);
	gbuf_grow(buf, len);
	memmove(buf->buffer + pos + len, buf->buffer + pos, buf->len - pos);
	buf->len += len;
}

void gbuf_remove(struct gbuf *buf, long pos, long len)
{
	BUG_ON(pos + len > buf->len);
	memmove(buf->buffer + pos, buf->buffer + pos + len, buf->len - pos - len);
	buf->len -= len;
}
