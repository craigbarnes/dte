#include "../../build/feature.h"
#include <unistd.h>
#include "winsize.h"

#if defined(HAVE_IOCTL_WINSIZE)

#include <sys/ioctl.h>

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

#elif defined(HAVE_TCGETWINSIZE)

#include <termios.h>

bool term_get_size(unsigned int *w, unsigned int *h)
{
    struct winsize ws;
    if (tcgetwinsize(STDIN_FILENO, &ws) != -1) {
        *w = ws.ws_col;
        *h = ws.ws_row;
        return true;
    }
    return false;
}

#else

bool term_get_size(unsigned int* UNUSED_ARG(w), unsigned int* UNUSED_ARG(h))
{
    return false;
}

#endif
