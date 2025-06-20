#ifndef UTIL_CONTAINER_H
#define UTIL_CONTAINER_H

#include "macros.h"

typedef void (*FreeFunction)(void *ptr);
#define FREE_FUNC(f) (FreeFunction)f

// Call a FreeFunction declared with an arbitrary pointer parameter
// type, without -fsanitize=function pedantry
NO_SANITIZE("undefined")
static inline void do_free_value(FreeFunction freefunc, void *ptr)
{
    freefunc(ptr);
}

#endif
