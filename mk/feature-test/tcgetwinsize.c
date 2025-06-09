#include <termios.h>

/*
 Testing for: tcgetwinsize()
 Supported by: Linux/musl, NetBSD, FreeBSD
 Alternative to: TIOCGWINSZ ioctl()
 Standardized by: POSIX 2024

 See also:
 • https://pubs.opengroup.org/onlinepubs/9799919799/functions/tcgetwinsize.html#:~:text=First%20released%20in%20Issue%208
 • https://www.austingroupbugs.net/view.php?id=1151
 • https://git.musl-libc.org/cgit/musl/commit/?id=4d5786544bb52c62fc1ae84d91684ef2268afa05
 • https://man.freebsd.org/cgi/man.cgi?query=tcgetwinsize#:~:text=int-,tcgetwinsize
 • https://man.netbsd.org/tcgetwinsize.3#:~:text=int-,tcgetwinsize
 • https://github.com/NetBSD/src/commit/e29ae9b278a89f738f6975a03efee357a669ca41
*/

int main(void)
{
    struct winsize ws;
    int r = (tcgetwinsize)(0, &ws);
    return !(r != -1 && ws.ws_col > 0 && ws.ws_row > 0);
}
