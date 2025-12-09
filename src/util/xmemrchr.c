#include "build-defs.h"
#include <string.h>
#include "xmemrchr.h"

void *xmemrchr(const void *ptr, int c, size_t n)
{
#if HAVE_MEMRCHR
    // See also: the comment at the top of src/util/xstring.h
    return likely(n) ? memrchr(ptr, c, n) : NULL;
#endif

    for (const unsigned char *s = ptr; n--; ) {
        if (s[n] == (unsigned char)c) {
            return (void*)(s + n);
        }
    }

    return NULL;
}
