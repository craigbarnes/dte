#include "defs.h"
#include <fcntl.h>
#include <stdlib.h>

int main(void)
{
    int fd = (mkostemp)("tmp-XXXXXX", O_CLOEXEC);
    return (fd < 0);
}
