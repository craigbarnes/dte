#include <errno.h>
#include <string.h>
#include "ptr-array.h"

static void ptr_array_grow(PointerArray *array)
{
    BUG_ON(array->alloc < array->count);
    const size_t alloc_min = 8;
    size_t alloc = MAX((array->alloc * 3) / 2, alloc_min);
    FATAL_ERROR_ON(alloc <= array->count, EOVERFLOW);
    array->ptrs = xrenew(array->ptrs, alloc);
    array->alloc = alloc;
}

// This is separate from ptr_array_append(), to allow the hot path to be
// inlined. It also duplicates the append operation so as to allow tail
// calling, which appears to improve code generation.
void ptr_array_grow_and_append(PointerArray *array, void *ptr)
{
    ptr_array_grow(array);
    array->ptrs[array->count++] = ptr;
}

// Move a pointer from one index to another
void ptr_array_move(PointerArray *array, size_t from, size_t to)
{
    BUG_ON(from >= array->count);
    BUG_ON(to >= array->count);
    if (unlikely(from == to)) {
        return;
    }

    void **p = array->ptrs;
    bool up = (from < to);
    void *dest = up ? p + from : p + to + 1;
    void *src = up ? p + from + 1 : p + to;
    size_t difference = up ? to - from : from - to;

    void *tmp = p[from];
    memmove(dest, src, difference * sizeof(void*));
    p[to] = tmp;
}

void ptr_array_insert(PointerArray *array, void *ptr, size_t idx)
{
    size_t last = array->count;
    BUG_ON(idx > last);
    ptr_array_append(array, ptr); // last is now array->count - 1
    ptr_array_move(array, last, idx);
}

// Free and remove and all pointers, without freeing the array itself
void ptr_array_clear(PointerArray *array, FreeFunction free_ptr)
{
    for (size_t i = 0, n = array->count; i < n; i++) {
        do_free_value(free_ptr, array->ptrs[i]);
        array->ptrs[i] = NULL;
    }
    array->count = 0;
}

void ptr_array_free_cb(PointerArray *array, FreeFunction free_ptr)
{
    ptr_array_clear(array, free_ptr);
    ptr_array_free_array(array);
}

size_t ptr_array_remove(PointerArray *array, void *ptr)
{
    size_t idx = ptr_array_xindex(array, ptr);
    void *removed = ptr_array_remove_index(array, idx);
    BUG_ON(removed != ptr);
    return idx;
}

void *ptr_array_remove_index(PointerArray *array, size_t idx)
{
    BUG_ON(idx >= array->count);
    void **ptrs = array->ptrs + idx;
    void *removed = *ptrs;
    memmove(ptrs, ptrs + 1, (--array->count - idx) * sizeof(void*));
    return removed;
}

// Return the first array index found to contain `ptr`, or SIZE_MAX
// if not found. See also: ptr_array_xindex(), ptr_array_bsearch().
size_t ptr_array_index(const PointerArray *array, const void *ptr)
{
    for (size_t i = 0, n = array->count; i < n; i++) {
        if (array->ptrs[i] == ptr) {
            return i;
        }
    }

    // If `ptr` isn't found in the array, return -1 (SIZE_MAX). Callers
    // should check that the returned index is less than `array->count`
    // before indexing `array->ptrs` with it or use ptr_array_xindex()
    // instead, if applicable.
    return -1;
}

// Trim all leading NULLs and all but one trailing NULL (if any)
void ptr_array_trim_nulls(PointerArray *array)
{
    size_t n = array->count;
    if (n == 0) {
        return;
    }

    void **ptrs = array->ptrs;
    while (n > 0 && ptrs[n - 1] == NULL) {
        n--;
    }

    if (n != array->count) {
        // Leave 1 trailing NULL
        n++;
    }

    if (n == 0 || ptrs[0] != NULL) {
        // No leading NULLs; simply adjust count and return early
        array->count = n;
        return;
    }

    size_t i = 0;
    while (i < n && ptrs[i] == NULL) {
        i++;
    }

    BUG_ON(i == 0);
    n -= i;
    array->count = n;
    memmove(ptrs, ptrs + i, n * sizeof(void*));
}
