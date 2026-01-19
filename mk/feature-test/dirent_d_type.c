#include <dirent.h>

/*
 Testing for: dirent::d_type struct member and DT_DIR/DT_LNK/etc. constants
 Supported by: Linux, OpenBSD, FreeBSD, NetBSD

 Note that the DT_* constants were added to <dirent.h> in POSIX 2024, but
 only for the new posix_dent struct and with no mention of dirent::d_type.

 See also:

 • https://man7.org/linux/man-pages/man3/readdir.3.html#:~:text=unsigned%20char-,d_type
 • https://man.openbsd.org/dir.5#:~:text=u_int8_t-,d_type
 • https://man.freebsd.org/cgi/man.cgi?query=dir&sektion=5#:~:text=__uint8_t-,d_type
 • https://man.netbsd.org/dirent.3#:~:text=uint8_t-,d_type
 • https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/dirent.h.html#:~:text=the%20structure%20posix_dent
*/

int main(void)
{
    const struct dirent de = {.d_type = DT_DIR};
    switch (de.d_type) {
    case DT_DIR:
        return 0;
    case DT_LNK:
    case DT_UNKNOWN:
        break;
    }

    return 1;
}
