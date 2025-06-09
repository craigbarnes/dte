#include "defs.h"
#include <fcntl.h>
#include <unistd.h>

/*
 Testing for: pipe2()
 Supported by: Linux, OpenBSD 5.7+, FreeBSD 10+, NetBSD 6+
 Standardized by: POSIX 2024

 See also:
 • https://pubs.opengroup.org/onlinepubs/9799919799/functions/pipe2.html#:~:text=adding%20pipe2
 • https://austingroupbugs.net/view.php?id=411
 • https://man7.org/linux/man-pages/man2/pipe.2.html#:~:text=int-,pipe2
 • https://man.openbsd.org/pipe2#:~:text=int-,pipe2
 • https://man.freebsd.org/cgi/man.cgi?query=pipe2#:~:int-,pipe2
 • https://man.netbsd.org/pipe2.2#:~:text=int-,pipe2
*/

int main(void)
{
    int fd[2] = {-1, -1};
    return (pipe2)(fd, O_CLOEXEC);
}
