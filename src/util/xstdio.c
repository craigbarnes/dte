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
