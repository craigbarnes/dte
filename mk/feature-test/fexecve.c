#include <unistd.h>

/*
 Testing for: fexecve()
 Supported by: Linux, FreeBSD, NetBSD
 Notably missing from: macOS, OpenBSD
 Standardized by: POSIX 2008

 See also:
 • https://pubs.opengroup.org/onlinepubs/9799919799/functions/fexecve.html#:~:text=fexecve()%20function%20is%20added
 • https://man7.org/linux/man-pages/man3/fexecve.3.html
 • https://man.freebsd.org/cgi/man.cgi?query=fexecve#:~:text=int-,fexecve
 • https://man.netbsd.org/fexecve.2#:~:text=int-,fexecve
*/

int main(void)
{
    static const char *const argv[] = {"argv0", "1", "2", NULL};
    static const char *const envp[] = {"VAR=xyz", NULL};
    return (fexecve)(3, (char**)argv, (char**)envp);
}
