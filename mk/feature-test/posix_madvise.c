#include <sys/mman.h>

int main(void)
{
    return (posix_madvise)(main, 64, POSIX_MADV_SEQUENTIAL);
}
