#include "defs.h"
#include <fcntl.h>
#include <unistd.h>

int feature_test_pipe2(int fd[2]);
int feature_test_pipe2(int fd[2])
{
    return pipe2(fd, O_CLOEXEC);
}
