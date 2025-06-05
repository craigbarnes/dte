#include <errno.h>
#include "arith.h"

size_t xmul_(size_t a, size_t b)
{
    size_t result;
    if (unlikely(size_multiply_overflows(a, b, &result))) {
        fatal_error(__func__, EOVERFLOW);
    }
    return result;
}

size_t xadd(size_t a, size_t b)
{
    size_t result;
    if (unlikely(size_add_overflows(a, b, &result))) {
        fatal_error(__func__, EOVERFLOW);
    }
    return result;
}
