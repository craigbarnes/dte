#include "../../build/feature.h" // Must be first include
#include <string.h>
#include "xmemmem.h"

void *xmemmem(const void *haystack, size_t hlen, const void *needle, size_t nlen)
{
#ifdef HAVE_MEMMEM
    return memmem(haystack, hlen, needle, nlen);
#else
    // TODO: fallback implementation
    #error "memmem(3) library function required"
#endif
}
