#include "feature.h"
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "ioctl.h"
#include "util/log.h"

#if HAVE_TIOCGWINSZ || HAVE_TIOCNOTTY
# include <sys/ioctl.h>
#endif

bool term_drop_controlling_tty(int fd)
{
#if HAVE_TIOCNOTTY
    return ioctl(fd, TIOCNOTTY) != -1;
#endif

    (void)fd;
    errno = ENOSYS;
    return false;
}

// NOLINTNEXTLINE(*-non-const-parameter)
bool term_get_size(unsigned int *w, unsigned int *h)
{
#if HAVE_TIOCGWINSZ || HAVE_TCGETWINSIZE
    struct winsize ws;
    #if HAVE_TIOCGWINSZ
        int r = ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
    #else
        int r = tcgetwinsize(STDIN_FILENO, &ws);
    #endif
    if (unlikely(r == -1)) {
        LOG_ERRNO("term_get_size");
        return false;
    }
    *w = ws.ws_col;
    *h = ws.ws_row;
    return true;
#endif

    (void)w;
    (void)h;
    return false;
}
