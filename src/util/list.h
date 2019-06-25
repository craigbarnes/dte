#ifndef UTIL_LIST_H
#define UTIL_LIST_H

#include <stddef.h> // offsetof
#include <stdbool.h>
#include "macros.h"

typedef struct ListHead {
    struct ListHead *next, *prev;
} ListHead;

static inline void list_init(ListHead *head)
{
    head->next = head;
    head->prev = head;
}

static inline void list_add(ListHead *new, ListHead *prev, ListHead *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_add_before(ListHead *new, ListHead *item)
{
    list_add(new, item->prev, item);
}

static inline void list_add_after(ListHead *new, ListHead *item)
{
    list_add(new, item, item->next);
}

static inline void list_del(ListHead *entry)
{
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
    entry->next = (void*)0x00100100;
    entry->prev = (void*)0x00200200;
}

static inline bool list_empty(const ListHead *const head)
{
    return head->next == head;
}

/**
 * container_of - cast a struct member out to the containing struct.
 * Uses GCC extensions, if available, to help catch type errors.
 * @ptr: a pointer to the member.
 * @type: the type of struct the member is embedded in.
 * @member: the name of the member within the struct.
 */
#if GNUC_AT_LEAST(3, 0)
#define container_of(ptr, type, member) __extension__ ({ \
    const __typeof__(((type*)0)->member) *mptr = (ptr); \
    (type*)((char*)mptr - offsetof(type, member)); \
})
#else
#define container_of(ptr, type, member) ( \
    (type*)((char*)(ptr) - offsetof(type, member)) \
)
#endif

/**
 * list_for_each_entry - iterate over list of given type
 * @pos: the type * to use as a loop counter.
 * @head: the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define list_for_each_entry(pos, head, member) \
    for ( \
        pos = container_of((head)->next, __typeof__(*pos), member); \
        &pos->member != (head); \
        pos = container_of(pos->member.next, __typeof__(*pos), member) \
    )

#endif
