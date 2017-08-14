#ifndef PTR_ARRAY_H
#define PTR_ARRAY_H

#include <stdlib.h>

typedef struct {
    void **ptrs;
    long alloc;
    long count;
} PointerArray;

#define PTR_ARRAY_INIT { \
    .ptrs = NULL, \
    .alloc = 0, \
    .count = 0 \
}

typedef void (*free_func)(void *ptr);
#define FREE_FUNC(f) (free_func)f

void ptr_array_add(PointerArray *array, void *ptr);
void ptr_array_insert(PointerArray *array, void *ptr, long pos);
void ptr_array_free_cb(PointerArray *array, free_func free_ptr);
void ptr_array_remove(PointerArray *array, void *ptr);
void *ptr_array_remove_idx(PointerArray *array, long pos);
long ptr_array_idx(PointerArray *array, void *ptr);
void *ptr_array_rel(PointerArray *array, void *ptr, long offset);

static inline void *ptr_array_next(PointerArray *array, void *ptr)
{
    return ptr_array_rel(array, ptr, 1);
}

static inline void *ptr_array_prev(PointerArray *array, void *ptr)
{
    return ptr_array_rel(array, ptr, -1);
}

static inline void ptr_array_free(PointerArray *array)
{
    ptr_array_free_cb(array, free);
}

#endif
