#ifndef UTIL_ALIGN_H
#define UTIL_ALIGN_H

#include <stddef.h> // size_t, max_align_t
#include <stdint.h> // intptr_t, intmax_t
#include "macros.h" // ALIGNAS(), ALIGNOF()

union MaxAlign {
    void *a;
    size_t b;
    long long c;
    intptr_t d;
    intmax_t e;
};

#if __STDC_VERSION__ >= 201112L
    // ISO C11 §6.7.5, §7.19
    #define MAXALIGN ALIGNAS(max_align_t)
#elif defined(HAS_ATTR_ALIGNED) && defined(__BIGGEST_ALIGNMENT__)
    // https://gcc.gnu.org/onlinedocs/gcc-4.4.7/gcc/Variable-Attributes.html#:~:text=__BIGGEST_ALIGNMENT__
    #define MAXALIGN ALIGNED(__BIGGEST_ALIGNMENT__)
#else
    #define MAXALIGN ALIGNAS(union MaxAlign)
#endif

#endif
