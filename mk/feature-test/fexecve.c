#include <unistd.h>

/*
 Testing for: fexecve()
 Supported by: Linux, FreeBSD, NetBSD
 Notably missing from: macOS, OpenBSD
 Standardized by: POSIX 2008

 See also:
 • https://man7.org/linux/man-pages/man3/fexecve.3.html
 • https://man.freebsd.org/cgi/man.cgi?query=fexecve#:~:text=int-,fexecve
 • https://man.netbsd.org/fexecve.2#:~:text=int-,fexecve
*/

extern char **environ;

int main(void)
{
    char *const argv[] = {"argv0", "1", "2", NULL};
    int r = (fexecve)(3, argv, environ);
    return !r;
}
