/**
 * list.h - Horizon kernel linked list definitions
 *
 * This file contains definitions for the linked list implementation.
 */

#ifndef _KERNEL_LIST_H
#define _KERNEL_LIST_H

#include <horizon/types.h>

/* List head structure */
typedef struct list_head {
    struct list_head *next;
    struct list_head *prev;
} list_head_t;

/* Initialize a list head */
static inline void list_init(list_head_t *list)
{
    list->next = list;
    list->prev = list;
}

/* Initialize a list head (macro version) */
#define INIT_LIST_HEAD(list) do { \
    (list)->next = (list); \
    (list)->prev = (list); \
} while (0)

/* List head initializer */
#define LIST_HEAD_INIT(name) { &(name), &(name) }

/* Add a new entry after the specified head */
static inline void list_add(list_head_t *new, list_head_t *head)
{
    head->next->prev = new;
    new->next = head->next;
    new->prev = head;
    head->next = new;
}

/* Add a new entry before the specified head */
static inline void list_add_tail(list_head_t *new, list_head_t *head)
{
    head->prev->next = new;
    new->prev = head->prev;
    new->next = head;
    head->prev = new;
}

/* Delete an entry from a list */
static inline void list_del(list_head_t *entry)
{
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
    entry->next = NULL;
    entry->prev = NULL;
}

/* Check if a list is empty */
static inline int list_empty(const list_head_t *head)
{
    return head->next == head;
}

/* Get the container of a list entry */
#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/* Iterate over a list */
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/* Iterate over a list safely (allows deletion) */
#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

/* Iterate over a list of a given type */
#define list_for_each_entry(pos, head, member) \
    for (pos = list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = list_entry(pos->member.next, typeof(*pos), member))

/* Iterate over a list of a given type safely (allows deletion) */
#define list_for_each_entry_safe(pos, n, head, member) \
    for (pos = list_entry((head)->next, typeof(*pos), member), \
         n = list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = n, n = list_entry(n->member.next, typeof(*pos), member))

/* Calculate the offset of a member in a structure */
#ifndef offsetof
#define offsetof(type, member) ((size_t)&((type *)0)->member)
#endif

#endif /* _KERNEL_LIST_H */
