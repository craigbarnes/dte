#include "defs.h"
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    return (dup3)(2, 1, O_CLOEXEC);
}
