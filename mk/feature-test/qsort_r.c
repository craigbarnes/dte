#include "defs.h"
#include <stdlib.h>

// https://austingroupbugs.net/view.php?id=900
// https://man7.org/linux/man-pages/man3/qsort.3.html
// https://git.musl-libc.org/cgit/musl/commit/?id=b76f37fd5625d038141b52184956fb4b7838e9a5
// https://man.freebsd.org/cgi/man.cgi?qsort(3)#HISTORY

static int cmp(const void *a, const void *b, void *ud)
{
    // This is just to avoid "unused parameter" warnings
    return (a > b) + !ud;
}

int main(void)
{
    char array[2][4] = {"xyz", "abc"};
    (qsort_r)(array, 2, 4, cmp, array);
    return 0;
}
