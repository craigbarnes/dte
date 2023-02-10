#include "defs.h"
#include <signal.h>
#include <string.h>

int main(void)
{
    const char *name = (sigabbrev_np)(SIGTERM);
    return !name;
}
