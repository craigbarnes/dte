#include <sys/ioctl.h>

int main(void)
{
    int r = (ioctl)(1, TIOCNOTTY);
    return r == -1;
}
