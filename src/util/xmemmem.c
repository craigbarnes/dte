#include "feature.h"
#include <string.h>
#include "xmemmem.h"
#include "debug.h"

void *xmemmem(const void *haystack, size_t hlen, const void *needle, size_t nlen)
{
    BUG_ON(nlen == 0);

#if HAVE_MEMMEM
    return memmem(haystack, hlen, needle, nlen);
#endif

    // Note: this fallback implementation isn't well suited to general
    // purpose use and can exhibit poor performance under certain
    // inputs. A library-quality memmem(3) is not a trivial thing to
    // implement, but fortunately almost every modern platform already
    // has one in libc and it's been added to the POSIX base spec in
    // issue 8[1]. Therefore, this code isn't likely to be used, and
    // even when it is, it should be acceptable for the uses in this
    // codebase.
    // [1]: https://www.austingroupbugs.net/view.php?id=1061

    const char *start = haystack;
    int first_char = ((const unsigned char*)needle)[0];
    if (nlen == 1) {
        return memchr(start, first_char, hlen);
    }

    while (1) {
        char *ptr = memchr(start, first_char, hlen);
        if (!ptr) {
            return NULL;
        }
        size_t skip = (size_t)(ptr - start);
        if (skip + nlen > hlen) {
            return NULL;
        }
        if (memcmp(ptr, needle, nlen) == 0) {
            return ptr;
        }
        start = ptr + 1;
        hlen -= skip + 1;
    }

    BUG("unexpected loop break");
    return NULL;
}
