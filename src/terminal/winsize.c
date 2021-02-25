#include "../../build/feature.h"
#include <unistd.h>
#include "winsize.h"

#if defined(HAVE_TIOCGWINSZ)

#include <sys/ioctl.h>

bool term_get_size(unsigned int *w, unsigned int *h)
{
    struct winsize ws;
    if (unlikely(ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1)) {
        return false;
    }
    *w = ws.ws_col;
    *h = ws.ws_row;
    return true;
}

#elif defined(HAVE_TCGETWINSIZE)

#include <termios.h>

bool term_get_size(unsigned int *w, unsigned int *h)
{
    struct winsize ws;
    if (unlikely(tcgetwinsize(STDIN_FILENO, &ws) != 0)) {
        return false;
    }
    *w = ws.ws_col;
    *h = ws.ws_row;
    return true;
}

#else

bool term_get_size(unsigned int* UNUSED_ARG(w), unsigned int* UNUSED_ARG(h))
{
    return false;
}

#endif
