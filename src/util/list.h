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

static inline bool list_empty(const ListHead *head)
{
    return head->next == head;
}

#endif
