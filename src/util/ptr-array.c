#include <string.h>
#include "ptr-array.h"

void ptr_array_add(PointerArray *array, void *ptr)
{
    size_t alloc = array->alloc;
    if (alloc == array->count) {
        // NOTE: if alloc was 1 then new alloc would be 1*3/2 = 1!
        alloc *= 3;
        alloc /= 2;
        if (alloc < 8) {
            alloc = 8;
        }
        xrenew(array->ptrs, alloc);
        array->alloc = alloc;
    }
    array->ptrs[array->count++] = ptr;
}

void ptr_array_insert(PointerArray *array, void *ptr, size_t pos)
{
    size_t count = array->count - pos;
    ptr_array_add(array, NULL);
    memmove(array->ptrs + pos + 1, array->ptrs + pos, count * sizeof(void *));
    array->ptrs[pos] = ptr;
}

void ptr_array_free_cb(PointerArray *array, FreeFunction free_ptr)
{
    for (size_t i = 0; i < array->count; i++) {
        free_ptr(array->ptrs[i]);
        array->ptrs[i] = NULL;
    }
    ptr_array_free_array(array);
}

void ptr_array_remove(PointerArray *array, void *ptr)
{
    size_t pos = ptr_array_idx(array, ptr);
    ptr_array_remove_idx(array, pos);
}

void *ptr_array_remove_idx(PointerArray *array, size_t pos)
{
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
    return -1;
}

void *ptr_array_rel(const PointerArray *array, const void *ptr, size_t offset)
{
    size_t i = ptr_array_idx(array, ptr);
    return array->ptrs[(i + offset + array->count) % array->count];
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
