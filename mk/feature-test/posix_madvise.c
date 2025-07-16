#include <sys/mman.h>

/*
 Testing for: posix_madvise()
 Supported by: Linux, OpenBSD, FreeBSD, NetBSD
 Standardized by: POSIX 2001 (Advisory Information option; "[ADV]")

 See also:
 • https://pubs.opengroup.org/onlinepubs/9799919799/functions/posix_madvise.html
 • https://man7.org/linux/man-pages/man3/posix_madvise.3.html
 • https://man.openbsd.org/posix_madvise#:~:text=int-,posix_madvise
 • https://man.freebsd.org/cgi/man.cgi?query=posix_madvise#:~:text=int-,posix_madvise
 • https://man.netbsd.org/posix_madvise.2#:~:text=int-,posix_madvise
*/

int main(void)
{
    static char buf[256];
    return (posix_madvise)(buf, sizeof buf, POSIX_MADV_SEQUENTIAL);
}
