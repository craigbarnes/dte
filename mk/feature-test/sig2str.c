#include <signal.h>

int main(void)
{
    char buf[SIG2STR_MAX];
    int r = (sig2str)(SIGTERM, buf);
    return !!r;
}
