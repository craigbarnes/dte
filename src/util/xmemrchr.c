#include "feature.h"
#include <string.h>
#include "xmemrchr.h"

void *xmemrchr(const void *ptr, int c, size_t n)
{
#if HAVE_MEMRCHR
    return memrchr(ptr, c, n);
#endif

    for (const unsigned char *s = ptr; n--; ) {
        if (s[n] == (unsigned char)c) {
            return (void*)(s + n);
        }
    }

    return NULL;
}
