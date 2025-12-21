#include "build-defs.h"
#include <termios.h>
#include <unistd.h>
#include "ioctl.h"

#if HAVE_TIOCGWINSZ || HAVE_TIOCNOTTY
# include <sys/ioctl.h>
#endif

SystemErrno term_drop_controlling_tty(int fd)
{
#if HAVE_TIOCNOTTY
    int r = ioctl(fd, TIOCNOTTY);
    return (r == -1) ? errno : 0;
#endif

    (void)fd;
    return ENOSYS;
}

// NOLINTNEXTLINE(*-non-const-parameter)
SystemErrno term_get_size(unsigned int *w, unsigned int *h)
{
#if HAVE_TIOCGWINSZ || HAVE_TCGETWINSIZE
    struct winsize ws;
    #if HAVE_TIOCGWINSZ
        int r = ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
    #else
        int r = tcgetwinsize(STDIN_FILENO, &ws);
    #endif
    if (unlikely(r == -1)) {
        return errno;
    }
    *w = ws.ws_col;
    *h = ws.ws_row;
    return 0;
#endif

    (void)w;
    (void)h;
    return ENOSYS;
}
