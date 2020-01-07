#include "defs.h"
#include <unistd.h>

int main(void)
{
    return (fsync)(1);
}
