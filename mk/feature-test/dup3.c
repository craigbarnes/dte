#include "defs.h"
#include <fcntl.h>
#include <unistd.h>

int feature_test_dup3(int oldfd, int newfd);
int feature_test_dup3(int oldfd, int newfd)
{
    return dup3(oldfd, newfd, O_CLOEXEC);
}
