#include <errno.h>
#include <string.h>
#include "ptr-array.h"

void ptr_array_append(PointerArray *array, void *ptr)
{
    size_t alloc = array->alloc;
    if (unlikely(alloc == array->count)) {
        const size_t ALLOC_MIN = 8;
        alloc = MAX((alloc * 3) / 2, ALLOC_MIN);
        if (unlikely(alloc <= array->count)) {
            fatal_error(__func__, EOVERFLOW);
        }
        xrenew(array->ptrs, alloc);
        array->alloc = alloc;
    }
    array->ptrs[array->count++] = ptr;
}

void ptr_array_insert(PointerArray *array, void *ptr, size_t pos)
{
    BUG_ON(pos > array->count);
    size_t count = array->count - pos;
    ptr_array_append(array, NULL);
    memmove(array->ptrs + pos + 1, array->ptrs + pos, count * sizeof(void*));
    array->ptrs[pos] = ptr;
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
    void *dest, *src;
    size_t difference;
    if (from < to) {
        dest = p + from;
        src = p + from + 1;
        difference = to - from;
    } else {
        dest = p + to + 1;
        src = p + to;
        difference = from - to;
    }

    void *tmp = p[from];
    memmove(dest, src, difference * sizeof(void*));
    p[to] = tmp;
}

void ptr_array_free_cb(PointerArray *array, FreeFunction free_ptr)
{
    for (size_t i = 0, n = array->count; i < n; i++) {
        free_ptr(array->ptrs[i]);
        array->ptrs[i] = NULL;
    }
    ptr_array_free_array(array);
}

size_t ptr_array_remove(PointerArray *array, void *ptr)
{
    size_t pos = ptr_array_idx(array, ptr);
    ptr_array_remove_idx(array, pos);
    return pos;
}

void *ptr_array_remove_idx(PointerArray *array, size_t pos)
{
    BUG_ON(pos >= array->count);
    void **ptrs = array->ptrs;
    void *removed = ptrs[pos];
    array->count--;
    memmove(ptrs + pos, ptrs + pos + 1, (array->count - pos) * sizeof(void*));
    return removed;
}

size_t ptr_array_idx(const PointerArray *array, const void *ptr)
{
    for (size_t i = 0, n = array->count; i < n; i++) {
        if (array->ptrs[i] == ptr) {
            return i;
        }
    }

    // If `ptr` isn't found in the array, return -1 (SIZE_MAX). Callers
    // should check that the returned index is less than `array->count`
    // before indexing `array->ptrs` with it or use a BUG_ON() assertion
    // for the same condition if `ptr` is always expected to be found.
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

    size_t i = 0;
    while (i < n && ptrs[i] == NULL) {
        i++;
    }

    if (i > 0) {
        n -= i;
        memmove(ptrs, ptrs + i, n * sizeof(void*));
    }

    array->count = n;
}
