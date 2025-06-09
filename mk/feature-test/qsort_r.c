#include "defs.h"
#include <stdlib.h>

/*
 Testing for: qsort_r()
 Supported by: Linux, FreeBSD 14+ (previously non-compatible), NetBSD
 Standardized by: POSIX 2024

 See also:
 • https://pubs.opengroup.org/onlinepubs/9799919799/functions/qsort.html#:~:text=adding%20the-,qsort_r
 • https://austingroupbugs.net/view.php?id=900
 • https://man7.org/linux/man-pages/man3/qsort.3.html#:~:text=void-,qsort_r
 • https://git.musl-libc.org/cgit/musl/commit/?id=b76f37fd5625d038141b52184956fb4b7838e9a5
 • https://man.freebsd.org/cgi/man.cgi?qsort_r#:~:text=void-,qsort_r
 • https://man.netbsd.org/qsort.3#:~:text=void-,qsort_r
*/

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
