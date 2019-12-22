#include "../../build/feature.h"
#include "winsize.h"

#ifdef HAVE_IOCTL_WINSIZE

#include <sys/ioctl.h>
#include <unistd.h>

bool term_get_size(unsigned int *w, unsigned int *h)
{
    struct winsize ws;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != -1) {
        *w = ws.ws_col;
        *h = ws.ws_row;
        return true;
    }
    return false;
}

#else

bool term_get_size(unsigned int *w, unsigned int *h)
{
    return false;
}

#endif
