#include "defs.h"
#include <signal.h>

/*
 Testing for: sigisemptyset(3)
 Supported by: Linux, FreeBSD

 See also:
 • https://man7.org/linux/man-pages/man3/sigisemptyset.3.html#VERSIONS:~:text=int-,sigisemptyset
 • https://man.freebsd.org/cgi/man.cgi?query=sigisemptyset#:~:text=int-,sigisemptyset
*/

int main(void)
{
    sigset_t set, old;
    sigemptyset(&set);
    sigprocmask(SIG_SETMASK, &set, &old);
    return (sigisemptyset)(&old);
}
