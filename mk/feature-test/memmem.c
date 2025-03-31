#include "defs.h"
#include <string.h>

/*
 memmem(3) was accepted into the POSIX specification in Issue 8 (2024), but
 since we target Issue 7 (2008) we nevertheless don't assume its presence.
 See also:
 • https://www.austingroupbugs.net/view.php?id=1061#:~:text=Resolution-,Accepted%20As%20Marked
 • https://pubs.opengroup.org/onlinepubs/9799919799/functions/memmem.html#:~:text=First%20released%20in%20Issue%208
*/

int main(void)
{
    static const char str[] = "finding a needle in a haystack";
    return (memmem)(str, sizeof(str) - 1, "needle", 6) == NULL;
}
