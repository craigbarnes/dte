#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string.h"
#include "utf8.h"
#include "xmalloc.h"
#include "../debug.h"

static void string_grow(String *s, size_t more)
{
    const size_t len = s->len + more;
    size_t alloc = s->alloc;
    if (alloc >= len) {
        return;
    }
    while (alloc < len) {
        alloc = (alloc * 3 + 2) / 2;
    }
    alloc = ROUND_UP(alloc, 16);
    xrenew(s->buffer, alloc);
    s->alloc = alloc;
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

void string_add_string_view(String *s, const StringView *sv)
{
    string_add_buf(s, sv->data, sv->length);
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

VPRINTF(2)
static void string_vsprintf(String *s, const char *fmt, va_list ap)
{
    va_list ap2;
    va_copy(ap2, ap);
    // Calculate the required size
    int n = vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);
    BUG_ON(n < 0);
    string_grow(s, n + 1);
    int wrote = vsnprintf(s->buffer + s->len, n + 1, fmt, ap);
    BUG_ON(wrote != n);
    s->len += wrote;
}

void string_sprintf(String *s, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    string_vsprintf(s, fmt, ap);
    va_end(ap);
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

char *string_clone_cstring(const String *s)
{
    const size_t len = s->len;
    char *b = xmalloc(len + 1);
    if (len > 0) {
        BUG_ON(!s->buffer);
        memcpy(b, s->buffer, len);
    }
    b[len] = '\0';
    return b;
}

void string_ensure_null_terminated(String *s)
{
    string_grow(s, 1);
    s->buffer[s->len] = '\0';
}

/*
 * This method first ensures that the String buffer is null-terminated
 * and then returns a const pointer to it, without doing any copying.
 *
 * NOTE: the returned pointer only remains valid so long as no other
 * methods are called on the String. There are no exceptions to this
 * rule. If the buffer is realloc'd, the pointer will be dangling and
 * using it will invoke undefined behaviour. If unsure, just use
 * string_clone_cstring() instead.
 */
const char *string_borrow_cstring(String *s)
{
    string_ensure_null_terminated(s);
    return s->buffer;
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
