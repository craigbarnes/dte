#ifndef UTIL_PTR_ARRAY_H
#define UTIL_PTR_ARRAY_H

#include <stdlib.h>
#include "macros.h"
#include "xmalloc.h"

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

typedef int (*CompareFunction)(const void *, const void *);
typedef void (*FreeFunction)(void *ptr);
#define FREE_FUNC(f) (FreeFunction)f

void ptr_array_add(PointerArray *array, void *ptr) NONNULL_ARG(1);
void ptr_array_insert(PointerArray *array, void *ptr, size_t pos) NONNULL_ARG(1);
void ptr_array_free_cb(PointerArray *array, FreeFunction free_ptr) NONNULL_ARGS;
void ptr_array_remove(PointerArray *array, void *ptr) NONNULL_ARG(1);
void *ptr_array_remove_idx(PointerArray *array, size_t pos) NONNULL_ARG(1);
size_t ptr_array_idx(const PointerArray *array, const void *ptr) NONNULL_ARG(1);
void *ptr_array_rel(const PointerArray *array, const void *ptr, size_t offset) NONNULL_ARG(1);
void ptr_array_trim_nulls(PointerArray *array) NONNULL_ARGS;

NONNULL_ARGS
static inline void ptr_array_init(PointerArray *array, size_t capacity)
{
    capacity = ROUND_UP(capacity, 8);
    array->count = 0;
    array->ptrs = capacity ? xnew(array->ptrs, capacity) : NULL;
    array->alloc = capacity;
}

NONNULL_ARG(1)
static inline void *ptr_array_next(const PointerArray *array, const void *ptr)
{
    return ptr_array_rel(array, ptr, 1);
}

NONNULL_ARG(1)
static inline void *ptr_array_prev(const PointerArray *array, const void *ptr)
{
    return ptr_array_rel(array, ptr, -1);
}

// Free each pointer and then free the array.
NONNULL_ARGS
static inline void ptr_array_free(PointerArray *array)
{
    ptr_array_free_cb(array, free);
}

// Free the array itself but not the pointers. Useful when the pointers
// are "borrowed" references.
NONNULL_ARGS
static inline void ptr_array_free_array(PointerArray *array)
{
    free(array->ptrs);
    *array = (PointerArray) PTR_ARRAY_INIT;
}

static inline void ptr_array_sort (
    const PointerArray *array,
    CompareFunction compare
) {
    if (array->count >= 2) {
        qsort(array->ptrs, array->count, sizeof(*array->ptrs), compare);
    }
}

static inline void *ptr_array_bsearch (
    const PointerArray array,
    const void *ptr,
    CompareFunction compare
) {
    return bsearch(&ptr, array.ptrs, array.count, sizeof(*array.ptrs), compare);
}

#endif
