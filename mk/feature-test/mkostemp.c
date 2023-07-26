#include "defs.h"
#include <fcntl.h> // O_CLOEXEC
#include <stdlib.h> // mkostemp(), glibc
#include <unistd.h> // mkostemp(), macOS

/*
 Note: for some reason macOS declares mkostemp() in <unistd.h>, despite
 the fact it originated in glibc, which declares it in <stdlib.h>.

 See also:

 - https://man7.org/linux/man-pages/man3/mkostemp.3.html
 - https://www.gnu.org/software/gnulib/manual/html_node/mkostemp.html
 - https://opensource.apple.com/source/Libc/Libc-1439.40.11/include/unistd.h.auto.html
*/

int main(void)
{
    char buf[] = "tmp-XXXXXX";
    int fd = (mkostemp)(buf, O_CLOEXEC);
    return (fd < 0);
}
