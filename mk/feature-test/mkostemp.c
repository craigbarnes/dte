#include "defs.h"
#include <fcntl.h> // O_CLOEXEC
#include <unistd.h> // mkostemp(): macOS
#include <stdlib.h> // mkostemp(): Linux, BSDs, POSIX 2024

/*
 Testing for: mkostemp()
 Supported by: Linux, OpenBSD 5.7+, FreeBSD 10+, NetBSD 7+, macOS
 Standardized by: POSIX 2024

 See also:
 • https://pubs.opengroup.org/onlinepubs/9799919799/functions/mkdtemp.html#:~:text=adding%20mkostemp
 • https://www.gnu.org/software/gnulib/manual/html_node/mkostemp.html
 • https://man7.org/linux/man-pages/man3/mkostemp.3.html
 • https://man.openbsd.org/mkostemp#:~:text=int-,mkostemp
 • https://man.freebsd.org/cgi/man.cgi?query=mkostemp#:~:text=int-,mkostemp
 • https://man.netbsd.org/mkstemp.3#:~:text=int-,mkostemp
*/

int main(void)
{
    char buf[] = "tmp-XXXXXX";
    int fd = (mkostemp)(buf, O_CLOEXEC);
    return (fd < 0);
}
