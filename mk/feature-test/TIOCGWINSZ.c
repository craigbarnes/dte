#include <sys/ioctl.h>

int main(void)
{
    struct winsize ws;
    int r = (ioctl)(0, TIOCGWINSZ, &ws);
    return !(r != -1 && ws.ws_col > 0 && ws.ws_row > 0);
}
