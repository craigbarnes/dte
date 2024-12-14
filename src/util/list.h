#ifndef UTIL_LIST_H
#define UTIL_LIST_H

#include <stdbool.h>
#include <stddef.h>
#include "macros.h"

typedef struct ListHead {
    struct ListHead *next, *prev;
} ListHead;

static inline void list_init(ListHead *head)
{
    head->next = head;
    head->prev = head;
}

static inline void list_insert(ListHead *new, ListHead *prev, ListHead *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_insert_before(ListHead *new, ListHead *item)
{
    list_insert(new, item->prev, item);
}

static inline void list_insert_after(ListHead *new, ListHead *item)
{
    list_insert(new, item, item->next);
}

static inline void list_remove(ListHead *entry)
{
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
    *entry = (ListHead){NULL, NULL};
}

static inline bool list_empty(const ListHead *head)
{
    return head->next == head;
}

#endif
