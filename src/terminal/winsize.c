#include <sys/ioctl.h>
#include "winsize.h"

bool term_get_size(unsigned int *w, unsigned int *h)
{
    struct winsize ws;
    if (ioctl(0, TIOCGWINSZ, &ws) != -1) {
        *w = ws.ws_col;
        *h = ws.ws_row;
        return true;
    }
    return false;
}
