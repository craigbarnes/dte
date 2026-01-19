#include "build-defs.h"
#include <sys/stat.h>
#include "xdirent.h"

MaybeBool is_dir_or_symlink_to_dir(const struct dirent *ent, int dir_fd)
{
    // The dirent::d_type field is a Linux/BSD extension and may always
    // be set to DT_UNKNOWN, depending on the filesystem. Thus, we need
    // check if the platform supports it (at compile-time) and whether
    // it provides useful information (at run-time). If either of these
    // is lacking or symbolic link resolution is needed, an extra call
    // to fstatat(3) is used as a fallback.

#if HAVE_DIRENT_D_TYPE
    switch (ent->d_type) {
        case DT_UNKNOWN: break; // Extra syscall needed for type info
        case DT_LNK: break; // Symlink (extra syscall needed to follow)
        case DT_DIR: return MB_TRUE; // Definitely a directory
        default: return MB_FALSE; // Definitely not a directory
    }
#endif

    struct stat st;
    if (fstatat(dir_fd, ent->d_name, &st, 0) != 0) {
        // If fstatat(3) fails then no determination can be made, since
        // `ent` could be e.g. a symlink to a directory that the calling
        // user has insufficient permissions for (errno == EACCES)
        return MB_INDETERMINATE;
    }

    return S_ISDIR(st.st_mode) ? MB_TRUE : MB_FALSE;
}
