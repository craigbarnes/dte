#include "defs.h"
#include <string.h>

int main(void)
{
    static const char str[] = "finding a needle in a haystack";
    return (memmem)(str, sizeof(str) - 1, "needle", 6) == NULL;
}
