#include <termios.h>

int main(void)
{
    struct winsize ws;
    int r = (tcgetwinsize)(0, &ws);
    return !(r != -1 && ws.ws_col > 0 && ws.ws_row > 0);
}
