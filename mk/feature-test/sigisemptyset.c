#include "defs.h"
#include <signal.h>

int main(void)
{
    sigset_t set, old;
    sigemptyset(&set);
    sigprocmask(SIG_SETMASK, &set, &old);
    return (sigisemptyset)(&old);
}
