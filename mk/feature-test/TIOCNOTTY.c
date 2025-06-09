#include <sys/ioctl.h>

/*
 Testing for: TIOCNOTTY ioctl() request
 Supported by: Linux, OpenBSD, FreeBSD, NetBSD

 See also:
 • https://man7.org/linux/man-pages/man2/TIOCSCTTY.2const.html#:~:text=TIOCNOTTY,-If
 • https://man7.org/linux/man-pages/man4/tty.4.html#:~:text=TIOCNOTTY,-Detach
 • https://man.openbsd.org/man4/tty.4#TIOCNOTTY
 • https://man.freebsd.org/cgi/man.cgi?query=tty&sektion=4#:~:text=TIOCNOTTY,-void
 • https://man.netbsd.org/tty.4#:~:text=TIOCNOTTY,-void
*/

int main(void)
{
    int r = (ioctl)(1, TIOCNOTTY);
    return r == -1;
}
