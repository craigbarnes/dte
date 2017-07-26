#ifndef PTR_ARRAY_H
#define PTR_ARRAY_H

#include <stdlib.h>

struct ptr_array {
	void **ptrs;
	long alloc;
	long count;
};

typedef void (*free_func)(void *ptr);

#define PTR_ARRAY(name) struct ptr_array name = { NULL, 0, 0 }
#define FREE_FUNC(f) (free_func)f

void ptr_array_add(struct ptr_array *array, void *ptr);
void ptr_array_insert(struct ptr_array *array, void *ptr, long pos);
void ptr_array_free_cb(struct ptr_array *array, free_func free_ptr);
void ptr_array_remove(struct ptr_array *array, void *ptr);
void *ptr_array_remove_idx(struct ptr_array *array, long pos);
long ptr_array_idx(struct ptr_array *array, void *ptr);
void *ptr_array_rel(struct ptr_array *array, void *ptr, long offset);

static inline void *ptr_array_next(struct ptr_array *array, void *ptr)
{
	return ptr_array_rel(array, ptr, 1);
}

static inline void *ptr_array_prev(struct ptr_array *array, void *ptr)
{
	return ptr_array_rel(array, ptr, -1);
}

static inline void ptr_array_free(struct ptr_array *array)
{
	ptr_array_free_cb(array, free);
}

#endif
