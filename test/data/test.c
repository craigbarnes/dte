#include <stdio.h>

int c_syntax_test(int argc, char *argv[])
{
    fprintf(stderr, "test: %s %c %zu\n", "str", 'x', (size_t)42);
    return 0;
}
