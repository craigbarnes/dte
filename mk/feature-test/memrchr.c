#include "defs.h"
#include <string.h>

int main(void)
{
    static const char str[] = "123456787654321";
    return (memrchr)(str, '7', sizeof(str) - 1) == NULL;
}
