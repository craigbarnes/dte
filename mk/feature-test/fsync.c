#include "defs.h"
#include <unistd.h>

/*
 Testing for: fsync()
 Supported by: Linux, all BSDs (from 4.2BSD)
 Standardized by: POSIX 2001 (File Synchronization option; "[FSC]")

 See also:
 • https://pubs.opengroup.org/onlinepubs/9799919799/functions/fsync.html
 • https://www.gnu.org/software/libc/manual/html_node/Synchronizing-I_002fO.html#index-fsync
 • https://man7.org/linux/man-pages/man3/fsync.3p.html
 • https://man7.org/linux/man-pages/man2/fsync.2.html
 • https://man.openbsd.org/fsync#:~:text=int-,fsync
 • https://man.freebsd.org/cgi/man.cgi?query=fsync#:~:text=int-,fsync
 • https://man.netbsd.org/fsync.2#:~:text=int-,fsync
*/

int main(void)
{
    return (fsync)(1);
}
