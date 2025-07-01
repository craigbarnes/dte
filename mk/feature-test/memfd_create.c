#include "defs.h"
#include <fcntl.h>
#include <sys/mman.h>

/*
 Testing for: memfd_create(), F_ADD_SEALS fcntl() command
 Supported by: Linux 3.17+, FreeBSD 13+

 See also:
 • https://man7.org/linux/man-pages/man2/memfd_create.2.html
 • https://man7.org/linux/man-pages/man2/fcntl.2.html#:~:text=F_ADD_SEALS,-(int
 • https://man.freebsd.org/cgi/man.cgi?query=memfd_create#:~:text=int-,memfd_create
 • https://man.freebsd.org/cgi/man.cgi?query=fcntl#:~:text=F_ADD_SEALS,-Add%20seals
*/

int main(void)
{
    int seals = F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE;
    int fd = (memfd_create)("example", MFD_ALLOW_SEALING);
    return fd == -1 || (fcntl)(fd, F_ADD_SEALS, seals) != 0;
}
