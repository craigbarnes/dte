// Stolen from Linux 2.6.7. Some functions renamed or removed.
#ifndef LIST_H
#define LIST_H

#include <stddef.h>

typedef struct list_head {
    struct list_head *next, *prev;
} ListHead;

#define LIST_HEAD_INIT(name) {&(name), &(name)}
#define LIST_HEAD(name) ListHead name = LIST_HEAD_INIT(name)

static inline void list_init(ListHead *head)
{
    head->next = head;
    head->prev = head;
}

static inline void __list_add(ListHead *new, ListHead *prev, ListHead *next) {
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_add_before(ListHead *new, ListHead *item)
{
    __list_add(new, item->prev, item);
}

static inline void list_add_after(ListHead *new, ListHead *item)
{
    __list_add(new, item, item->next);
}

static inline void __list_del(ListHead *prev, ListHead *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(ListHead *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = (void *)0x00100100;
    entry->prev = (void *)0x00200200;
}

static inline int list_empty(const ListHead *head)
{
    return head->next == head;
}

/**
 * container_of - cast a member of a structure out to the containing structure
 *
 * @ptr: the pointer to the member.
 * @type: the type of the container struct this is embedded in.
 * @member: the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({ \
    const __typeof__( ((type *)0)->member ) *__mptr = (ptr); \
    (type *)( (char *)__mptr - offsetof(type,member) );})

/**
 * list_for_each_entry - iterate over list of given type
 * @pos: the type * to use as a loop counter.
 * @head: the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define list_for_each_entry(pos, head, member) \
    for (pos = container_of((head)->next, __typeof__(*pos), member); \
         &pos->member != (head); \
         pos = container_of(pos->member.next, __typeof__(*pos), member))

#endif
