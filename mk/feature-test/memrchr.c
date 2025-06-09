#include "defs.h"
#include <string.h>

/*
 Testing for: memrchr(3)
 Supported by: Linux (glibc, musl), OpenBSD, FreeBSD, NetBSD

 See also:
 • https://man7.org/linux/man-pages/man3/memrchr.3.html
 • https://git.musl-libc.org/cgit/musl/commit/?id=6597f9ac133fd4f47dea307d6260fd52eae77816
 • https://man.openbsd.org/memrchr.3
 • https://man.freebsd.org/cgi/man.cgi?query=memrchr&sektion=3
 • https://man.netbsd.org/memrchr.3
*/

int main(void)
{
    static const char str[] = "123456787654321";
    return (memrchr)(str, '7', sizeof(str) - 1) == NULL;
}
