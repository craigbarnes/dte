#include <stdio.h>

int c_syntax_test(int argc, char *argv[])
{
    fprintf(stderr, "test: %s %c %d\n", "str", 'x', 42);
    return 0;
}
