#include <sys/ioctl.h>

/*
 Testing for: TIOCGWINSZ ioctl() request
 Supported by: Linux, OpenBSD, FreeBSD, NetBSD, macOS

 See also:
 • https://man7.org/linux/man-pages/man2/TIOCGWINSZ.2const.html#:~:text=TIOCGWINSZ,-Get
 • https://man.openbsd.org/man4/tty.4#TIOCGWINSZ
 • https://man.freebsd.org/cgi/man.cgi?query=tty&sektion=4#:~:text=TIOCGWINSZ,-struct
 • https://man.netbsd.org/tty.4#:~:text=TIOCGWINSZ,-struct
*/

int main(void)
{
    struct winsize ws;
    int r = (ioctl)(0, TIOCGWINSZ, &ws);
    return !(r != -1 && ws.ws_col > 0 && ws.ws_row > 0);
}
