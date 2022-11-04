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
