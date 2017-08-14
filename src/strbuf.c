#include "strbuf.h"
#include "common.h"
#include "uchar.h"

void strbuf_grow(StringBuffer *buf, long more)
{
    long alloc = ROUND_UP(buf->len + more, 16);

    if (alloc > buf->alloc) {
        buf->alloc = alloc;
        buf->buffer = xrealloc(buf->buffer, buf->alloc);
    }
}

void strbuf_free(StringBuffer *buf)
{
    free(buf->buffer);
    strbuf_init(buf);
}

void strbuf_add_byte(StringBuffer *buf, unsigned char byte)
{
    strbuf_grow(buf, 1);
    buf->buffer[buf->len++] = byte;
}

long strbuf_add_ch(StringBuffer *buf, unsigned int u)
{
    unsigned int len = u_char_size(u);

    strbuf_grow(buf, len);
    u_set_char_raw(buf->buffer, &buf->len, u);
    return len;
}

long strbuf_insert_ch(StringBuffer *buf, long pos, unsigned int u)
{
    unsigned int len = u_char_size(u);

    strbuf_make_space(buf, pos, len);
    u_set_char_raw(buf->buffer, &pos, u);
    return len;
}

void strbuf_add_str(StringBuffer *buf, const char *str)
{
    strbuf_add_buf(buf, str, strlen(str));
}

void strbuf_add_buf(StringBuffer *buf, const char *ptr, long len)
{
    if (!len)
        return;
    strbuf_grow(buf, len);
    memcpy(buf->buffer + buf->len, ptr, len);
    buf->len += len;
}

char *strbuf_steal(StringBuffer *buf, long *len)
{
    char *b = buf->buffer;
    *len = buf->len;
    strbuf_init(buf);
    return b;
}

char *strbuf_steal_cstring(StringBuffer *buf)
{
    char *b;
    strbuf_add_byte(buf, 0);
    b = buf->buffer;
    strbuf_init(buf);
    return b;
}

char *strbuf_cstring(StringBuffer *buf)
{
    char *b = xnew(char, buf->len + 1);
    if (buf->len > 0) {
        BUG_ON(!buf->buffer);
        memcpy(b, buf->buffer, buf->len);
    }
    b[buf->len] = '\0';
    return b;
}

void strbuf_make_space(StringBuffer *buf, long pos, long len)
{
    BUG_ON(pos > buf->len);
    strbuf_grow(buf, len);
    memmove(buf->buffer + pos + len, buf->buffer + pos, buf->len - pos);
    buf->len += len;
}

void strbuf_remove(StringBuffer *buf, long pos, long len)
{
    BUG_ON(pos + len > buf->len);
    memmove(buf->buffer + pos, buf->buffer + pos + len, buf->len - pos - len);
    buf->len -= len;
}
