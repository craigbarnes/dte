#include "string.h"
#include "common.h"
#include "uchar.h"

void string_grow(String *buf, size_t more)
{
    size_t alloc = ROUND_UP(buf->len + more, 16);

    if (alloc > buf->alloc) {
        buf->alloc = alloc;
        buf->buffer = xrealloc(buf->buffer, buf->alloc);
    }
}

void string_free(String *buf)
{
    free(buf->buffer);
    string_init(buf);
}

void string_add_byte(String *buf, unsigned char byte)
{
    string_grow(buf, 1);
    buf->buffer[buf->len++] = byte;
}

size_t string_add_ch(String *buf, CodePoint u)
{
    size_t len = u_char_size(u);

    string_grow(buf, len);
    u_set_char_raw(buf->buffer, &buf->len, u);
    return len;
}

size_t string_insert_ch(String *buf, size_t pos, CodePoint u)
{
    size_t len = u_char_size(u);

    string_make_space(buf, pos, len);
    u_set_char_raw(buf->buffer, &pos, u);
    return len;
}

void string_add_str(String *buf, const char *str)
{
    string_add_buf(buf, str, strlen(str));
}

void string_add_buf(String *buf, const char *ptr, size_t len)
{
    if (!len) {
        return;
    }
    string_grow(buf, len);
    memcpy(buf->buffer + buf->len, ptr, len);
    buf->len += len;
}

char *string_steal(String *buf, size_t *len)
{
    char *b = buf->buffer;
    *len = buf->len;
    string_init(buf);
    return b;
}

char *string_steal_cstring(String *buf)
{
    char *b;
    string_add_byte(buf, 0);
    b = buf->buffer;
    string_init(buf);
    return b;
}

char *string_cstring(String *buf)
{
    char *b = xnew(char, buf->len + 1);
    if (buf->len > 0) {
        BUG_ON(!buf->buffer);
        memcpy(b, buf->buffer, buf->len);
    }
    b[buf->len] = '\0';
    return b;
}

void string_make_space(String *buf, size_t pos, size_t len)
{
    BUG_ON(pos > buf->len);
    string_grow(buf, len);
    memmove(buf->buffer + pos + len, buf->buffer + pos, buf->len - pos);
    buf->len += len;
}

void string_remove(String *buf, size_t pos, size_t len)
{
    BUG_ON(pos + len > buf->len);
    memmove(buf->buffer + pos, buf->buffer + pos + len, buf->len - pos - len);
    buf->len -= len;
}
