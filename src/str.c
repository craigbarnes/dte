#include "str.h"
#include "common.h"
#include "uchar.h"

static void string_grow(String *s, size_t more)
{
    size_t alloc = ROUND_UP(s->len + more, 16);

    if (alloc > s->alloc) {
        s->alloc = alloc;
        s->buffer = xrealloc(s->buffer, s->alloc);
    }
}

void string_free(String *s)
{
    free(s->buffer);
    string_init(s);
}

void string_add_byte(String *s, unsigned char byte)
{
    string_grow(s, 1);
    s->buffer[s->len++] = byte;
}

size_t string_add_ch(String *s, CodePoint u)
{
    size_t len = u_char_size(u);

    string_grow(s, len);
    u_set_char_raw(s->buffer, &s->len, u);
    return len;
}

size_t string_insert_ch(String *s, size_t pos, CodePoint u)
{
    size_t len = u_char_size(u);

    string_make_space(s, pos, len);
    u_set_char_raw(s->buffer, &pos, u);
    return len;
}

void string_add_str(String *s, const char *str)
{
    string_add_buf(s, str, strlen(str));
}

void string_add_buf(String *s, const char *ptr, size_t len)
{
    if (!len) {
        return;
    }
    string_grow(s, len);
    memcpy(s->buffer + s->len, ptr, len);
    s->len += len;
}

char *string_steal(String *s, size_t *len)
{
    char *b = s->buffer;
    *len = s->len;
    string_init(s);
    return b;
}

char *string_steal_cstring(String *s)
{
    string_add_byte(s, '\0');
    char *b = s->buffer;
    string_init(s);
    return b;
}

char *string_cstring(const String *s)
{
    char *b = xnew(char, s->len + 1);
    if (s->len > 0) {
        BUG_ON(!s->buffer);
        memcpy(b, s->buffer, s->len);
    }
    b[s->len] = '\0';
    return b;
}

void string_make_space(String *s, size_t pos, size_t len)
{
    BUG_ON(pos > s->len);
    string_grow(s, len);
    memmove(s->buffer + pos + len, s->buffer + pos, s->len - pos);
    s->len += len;
}

void string_remove(String *s, size_t pos, size_t len)
{
    BUG_ON(pos + len > s->len);
    memmove(s->buffer + pos, s->buffer + pos + len, s->len - pos - len);
    s->len -= len;
}
