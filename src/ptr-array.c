#include "ptr-array.h"
#include "xmalloc.h"

#include <string.h>

void ptr_array_add(struct ptr_array *array, void *ptr)
{
	if (array->count == array->alloc) {
		// NOTE: if alloc was 1 then new alloc would be 1*3/2 = 1!
		array->alloc *= 3;
		array->alloc /= 2;
		if (array->alloc < 8) {
			array->alloc = 8;
		}
		xrenew(array->ptrs, array->alloc);
	}
	array->ptrs[array->count++] = ptr;
}

void ptr_array_insert(struct ptr_array *array, void *ptr, long pos)
{
	long count = array->count - pos;
	ptr_array_add(array, NULL);
	memmove(array->ptrs + pos + 1, array->ptrs + pos, count * sizeof(void *));
	array->ptrs[pos] = ptr;
}

void ptr_array_free_cb(struct ptr_array *array, free_func free_ptr)
{
	int i;

	for (i = 0; i < array->count; i++) {
		free_ptr(array->ptrs[i]);
		array->ptrs[i] = NULL;
	}
	free(array->ptrs);
	array->ptrs = NULL;
	array->alloc = 0;
	array->count = 0;
}

void ptr_array_remove(struct ptr_array *array, void *ptr)
{
	long pos = ptr_array_idx(array, ptr);
	ptr_array_remove_idx(array, pos);
}

void *ptr_array_remove_idx(struct ptr_array *array, long pos)
{
	void *ptr = array->ptrs[pos];
	array->count--;
	memmove(array->ptrs + pos, array->ptrs + pos + 1, (array->count - pos) * sizeof(void *));
	return ptr;
}

long ptr_array_idx(struct ptr_array *array, void *ptr)
{
	long i;

	for (i = 0; i < array->count; i++) {
		if (array->ptrs[i] == ptr)
			return i;
	}
	return -1;
}

void *ptr_array_rel(struct ptr_array *array, void *ptr, long offset)
{
	long i = ptr_array_idx(array, ptr);

	if (i < 0) {
		return NULL;
	}
	return array->ptrs[(i + offset + array->count) % array->count];
}
