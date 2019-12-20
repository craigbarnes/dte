#include "defs.h"
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd[2] = {-1, -1};
    return (pipe2)(fd, O_CLOEXEC);
}
