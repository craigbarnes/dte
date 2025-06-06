#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "arith.h"
#include "debug.h"
#include "utf8.h"
#include "xmalloc.h"

static COLD void string_grow(String *s, size_t min_alloc)
{
    size_t alloc = s->alloc;
    while (alloc < min_alloc) {
        alloc = (alloc * 3 + 2) / 2;
    }
    alloc = next_multiple(alloc, 16);
    s->alloc = alloc;
    s->buffer = xrealloc(s->buffer, alloc);
}

// Reserve `more` bytes of additional space and return a pointer to the
// start of it, to allow writing directly to the buffer. Updating `s->len`
// to reflect the new length is left up to the caller.
char *string_reserve_space(String *s, size_t more)
{
    BUG_ON(more == 0);
    size_t min_alloc = xadd(s->len, more);
    if (unlikely(s->alloc < min_alloc)) {
        string_grow(s, min_alloc);
    }
    return s->buffer + s->len;
}

void string_free(String *s)
{
    free(s->buffer);
    *s = (String) STRING_INIT;
}

void string_append_byte(String *s, unsigned char byte)
{
    string_reserve_space(s, 1);
    s->buffer[s->len++] = byte;
}

size_t string_append_codepoint(String *s, CodePoint u)
{
    string_reserve_space(s, UTF8_MAX_SEQ_LEN);
    size_t n = u_set_char_raw(s->buffer + s->len, u);
    s->len += n;
    return n;
}

static char *string_make_space(String *s, size_t pos, size_t len)
{
    BUG_ON(pos > s->len);
    BUG_ON(len == 0);
    string_reserve_space(s, len);
    char *start = s->buffer + pos;
    memmove(start + len, start, s->len - pos);
    s->len += len;
    return start;
}

size_t string_insert_codepoint(String *s, size_t pos, CodePoint u)
{
    size_t len = u_char_size(u);
    return u_set_char_raw(string_make_space(s, pos, len), u);
}

void string_insert_buf(String *s, size_t pos, const char *buf, size_t len)
{
    if (len == 0) {
        return;
    }
    memcpy(string_make_space(s, pos, len), buf, len);
}

void string_append_buf(String *s, const char *ptr, size_t len)
{
    if (len == 0) {
        return;
    }
    char *reserved = string_reserve_space(s, len);
    s->len += len;
    memcpy(reserved, ptr, len);
}

void string_append_memset(String *s, unsigned char byte, size_t len)
{
    if (len == 0) {
        return;
    }
    char *reserved = string_reserve_space(s, len);
    s->len += len;
    memset(reserved, byte, len);
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
    string_reserve_space(s, n + 1);
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

static char *null_terminate(String *s)
{
    string_reserve_space(s, 1);
    s->buffer[s->len] = '\0';
    return s->buffer;
}

char *string_steal_cstring(String *s)
{
    char *buf = null_terminate(s);
    *s = (String) STRING_INIT;
    return buf;
}

char *string_clone_cstring(const String *s)
{
    size_t len = s->len;
    char *b = xmalloc(len + 1);
    b[len] = '\0';
    return likely(len) ? memcpy(b, s->buffer, len) : b;
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
    return null_terminate(s);
}

void string_remove(String *s, size_t pos, size_t len)
{
    if (len == 0) {
        return;
    }
    size_t oldlen = s->len;
    BUG_ON(pos + len > oldlen);
    s->len -= len;
    memmove(s->buffer + pos, s->buffer + pos + len, oldlen - pos - len);
}
