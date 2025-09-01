#include "build-defs.h"
#include <sys/mman.h>
#include "xadvise.h"
#include "debug.h"

int advise_sequential(void *addr, size_t len)
{
    BUG_ON(len == 0);

#if HAVE_POSIX_MADVISE
    return posix_madvise(addr, len, POSIX_MADV_SEQUENTIAL);
#else
    (void)addr;
#endif

    // "The posix_madvise() function shall have no effect on the semantics
    // of access to memory in the specified range, although it may affect
    // the performance of access". Ergo, doing nothing is a valid fallback.
    return 0;
}
