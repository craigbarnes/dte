#ifndef UTIL_PTR_ARRAY_H
#define UTIL_PTR_ARRAY_H

#include <stdlib.h>
#include "debug.h"
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

void ptr_array_grow_and_append(PointerArray *array, void *ptr) NONNULL_ARG(1) NOINLINE;
void ptr_array_insert(PointerArray *array, void *ptr, size_t idx) NONNULL_ARG(1);
void ptr_array_move(PointerArray *array, size_t from, size_t to) NONNULL_ARGS;
void ptr_array_free_cb(PointerArray *array, FreeFunction free_ptr) NONNULL_ARGS;
size_t ptr_array_remove(PointerArray *array, void *ptr) NONNULL_ARG(1);
void *ptr_array_remove_index(PointerArray *array, size_t idx) NONNULL_ARGS;
size_t ptr_array_index(const PointerArray *array, const void *ptr) NONNULL_ARG(1);
void ptr_array_trim_nulls(PointerArray *array) NONNULL_ARGS;

NONNULL_ARGS
static inline void ptr_array_init(PointerArray *array, size_t capacity)
{
    capacity = round_size_to_next_multiple(capacity, 8);
    array->count = 0;
    array->ptrs = capacity ? xnew(array->ptrs, capacity) : NULL;
    array->alloc = capacity;
}

NONNULL_ARG(1)
static inline void ptr_array_append(PointerArray *array, void *ptr)
{
    if (unlikely(array->alloc <= array->count)) {
        ptr_array_grow_and_append(array, ptr);
        return;
    }
    array->ptrs[array->count++] = ptr;
}

// Like ptr_array_index(), but asserting that `ptr` should always be found
// in the array (i.e. is known to be present), instead of the caller doing so
static inline size_t ptr_array_xindex(const PointerArray *array, const void *ptr)
{
    size_t idx = ptr_array_index(array, ptr);
    BUG_ON(idx >= array->count);
    return idx;
}

// Swap the pointers at two indices
NONNULL_ARGS
static inline void ptr_array_swap(PointerArray *array, size_t a, size_t b)
{
    BUG_ON(a >= array->count);
    BUG_ON(b >= array->count);
    void **ptrs = array->ptrs;
    void *tmp = ptrs[a];
    ptrs[a] = ptrs[b];
    ptrs[b] = tmp;
}

// Free each pointer and then free the array
NONNULL_ARGS
static inline void ptr_array_free(PointerArray *array)
{
    ptr_array_free_cb(array, free);
}

// Free the array itself but not the pointers (useful when the
// pointers are "borrowed" references)
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
