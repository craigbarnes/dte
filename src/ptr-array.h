#ifndef PTR_ARRAY_H
#define PTR_ARRAY_H

#include <stdlib.h>

typedef struct {
    void **ptrs;
    size_t alloc;
    size_t count;
} PointerArray;

#define PTR_ARRAY_INIT { \
    .ptrs = NULL, \
    .alloc = 0, \
    .count = 0 \
}

typedef void (*free_func)(void *ptr);
#define FREE_FUNC(f) (free_func)f

void ptr_array_add(PointerArray *array, void *ptr);
void ptr_array_insert(PointerArray *array, void *ptr, size_t pos);
void ptr_array_free_cb(PointerArray *array, free_func free_ptr);
void ptr_array_remove(PointerArray *array, void *ptr);
void *ptr_array_remove_idx(PointerArray *array, size_t pos);
size_t ptr_array_idx(const PointerArray *array, const void *ptr);
void *ptr_array_rel(const PointerArray *array, const void *ptr, size_t offset);

static inline void *ptr_array_next(const PointerArray *array, const void *ptr)
{
    return ptr_array_rel(array, ptr, 1);
}

static inline void *ptr_array_prev(const PointerArray *array, const void *ptr)
{
    return ptr_array_rel(array, ptr, -1);
}

static inline void ptr_array_free(PointerArray *array)
{
    ptr_array_free_cb(array, free);
}

#endif
