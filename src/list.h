#ifndef LIST_H
#define LIST_H

#include <stddef.h> // offsetof
#include <stdbool.h>

typedef struct ListHead ListHead;

struct ListHead {
    ListHead *next, *prev;
};

static inline void list_init(ListHead *head)
{
    head->next = head;
    head->prev = head;
}

static inline void list_add(ListHead *new, ListHead *prev, ListHead *next) {
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
    entry->next = (void *)0x00100100;
    entry->prev = (void *)0x00200200;
}

static inline bool list_empty(const ListHead *const head)
{
    return head->next == head;
}

/**
 * container_of - cast a member of a structure out to the containing structure
 *
 * @param ptr: the pointer to the member.
 * @param type: the type of the container struct this is embedded in.
 * @param member: the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({ \
    const __typeof__( ((type *)0)->member ) *__mptr = (ptr); \
    (type *)( (char *)__mptr - offsetof(type,member) ); \
})

/**
 * list_for_each_entry - iterate over list of given type
 * @param pos: the type * to use as a loop counter.
 * @param head: the head for your list.
 * @param member: the name of the list_struct within the struct.
 */
#define list_for_each_entry(pos, head, member) \
    for ( \
        pos = container_of((head)->next, __typeof__(*pos), member); \
        &pos->member != (head); \
        pos = container_of(pos->member.next, __typeof__(*pos), member) \
    )

#endif
