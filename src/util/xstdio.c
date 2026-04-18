#include "xstdio.h"

char *xfgets(char *restrict buf, int bufsize, FILE *restrict stream)
{
    char *r;
    do {
        clearerr(stream);
        r = fgets(buf, bufsize, stream);
    } while (unlikely(!r && ferror(stream) && errno == EINTR));
    return r;
}

int xfputs(const char *restrict str, FILE *restrict stream)
{
    int r;
    do {
        r = fputs(str, stream);
    } while (unlikely(r == EOF && errno == EINTR));
    return r;
}

int xfputc(int c, FILE *stream)
{
    int r;
    do {
        r = fputc(c, stream);
    } while (unlikely(r == EOF && errno == EINTR));
    return r;
}

size_t xfwrite_all(const char *restrict buf, size_t nitems, FILE *restrict stream)
{
    // "The fwrite() function shall return the number of elements
    // successfully written, which shall be less than nitems only
    // if a write error is encountered."
    // -- https://pubs.opengroup.org/onlinepubs/9799919799/functions/fwrite.html
    size_t pos = 0;
    do {
        pos += fwrite(buf + pos, 1, nitems - pos, stream);
    } while (unlikely(pos < nitems && errno == EINTR));

    BUG_ON(pos > nitems);
    return pos;
}

int xvfprintf(FILE *restrict stream, const char *restrict fmt, va_list ap)
{
    int r;
    do {
        r = vfprintf(stream, fmt, ap);
    } while (unlikely(r < 0 && errno == EINTR));
    return r;
}

int xfprintf(FILE *restrict stream, const char *restrict fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = xvfprintf(stream, fmt, ap);
    va_end(ap);
    return r;
}

int xfflush(FILE *stream)
{
    int r;
    do {
        r = fflush(stream);
    } while (unlikely(r != 0 && errno == EINTR));
    return r;
}
