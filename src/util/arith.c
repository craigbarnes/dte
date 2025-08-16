#include <errno.h>
#include "arith.h"

size_t xmul_(size_t a, size_t b)
{
    size_t result;
    bool overflow = size_multiply_overflows(a, b, &result);
    FATAL_ERROR_ON(overflow, EOVERFLOW);
    return result;
}

size_t xadd(size_t a, size_t b)
{
    size_t result;
    bool overflow = size_add_overflows(a, b, &result);
    FATAL_ERROR_ON(overflow, EOVERFLOW);
    return result;
}
