#include "defs.h"
#include <fcntl.h>
#include <unistd.h>

/*
 Testing for: dup3()
 Supported by: Linux, OpenBSD 5.7+, FreeBSD 10+, NetBSD 6+
 Standardized by: POSIX 2024

 See also:
 • https://pubs.opengroup.org/onlinepubs/9799919799/functions/dup3.html#:~:text=adding%20dup3
 • https://austingroupbugs.net/view.php?id=411
 • https://man7.org/linux/man-pages/man2/dup.2.html#:~:text=int-,dup3
 • https://man.openbsd.org/dup3#:~:text=int-,dup3
 • https://man.freebsd.org/cgi/man.cgi?query=dup3#:~:text=int-,dup3
 • https://man.netbsd.org/dup3.2#:~:text=int-,dup3
*/

int main(void)
{
    return (dup3)(2, 1, O_CLOEXEC);
}
