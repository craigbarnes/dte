#include "defs.h"
#include <string.h>

/*
 Testing for: memmem(3)
 Supported by: Linux (glibc), OpenBSD, FreeBSD, NetBSD
 Standardized by: POSIX 2024

 See also:
 • https://pubs.opengroup.org/onlinepubs/9799919799/functions/memmem.html#:~:text=First%20released%20in%20Issue%208
 • https://www.austingroupbugs.net/view.php?id=1061#:~:text=Resolution-,Accepted%20As%20Marked
 • https://www.man7.org/linux/man-pages/man3/memmem.3.html
 • https://man.openbsd.org/memmem.3
 • https://man.freebsd.org/cgi/man.cgi?query=memmem&sektion=3
 • https://man.netbsd.org/memmem.3
*/

int main(void)
{
    static const char str[] = "finding a needle in a haystack";
    return (memmem)(str, sizeof(str) - 1, "needle", 6) == NULL;
}
