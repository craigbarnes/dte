#include <sys/mman.h>

int main(void)
{
    static char buf[256];
    return (posix_madvise)(buf, sizeof buf, POSIX_MADV_SEQUENTIAL);
}
