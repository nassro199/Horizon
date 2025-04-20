/**
 * wait.h - Horizon kernel wait queue definitions
 * 
 * This file contains definitions for wait queues.
 */

#ifndef _HORIZON_WAIT_H
#define _HORIZON_WAIT_H

#include <horizon/types.h>
#include <horizon/list.h>
#include <horizon/spinlock.h>

/* Wait queue entry */
typedef struct wait_queue_entry {
    unsigned int flags;            /* Flags */
    void *private;                 /* Private data */
    int (*func)(struct wait_queue_entry *wq_entry, unsigned mode, int flags, void *key);  /* Wakeup function */
    struct list_head link;         /* Link in wait queue */
} wait_queue_entry_t;

/* Wait queue head */
typedef struct wait_queue_head {
    spinlock_t lock;               /* Lock */
    struct list_head head;         /* List head */
} wait_queue_head_t;

/* Wait queue flags */
#define WQ_FLAG_EXCLUSIVE          0x01  /* Exclusive wait */
#define WQ_FLAG_WOKEN              0x02  /* Woken flag */

/* Wait queue initializers */
#define WAIT_QUEUE_HEAD_INITIALIZER { SPIN_LOCK_INITIALIZER, LIST_HEAD_INIT }
#define WAIT_QUEUE_HEAD_INITIALIZER_ONSTACK { SPIN_LOCK_INITIALIZER, LIST_HEAD_INIT }

/* Initialize a wait queue head */
static inline void wait_queue_init(wait_queue_head_t *wq_head) {
    spin_lock_init(&wq_head->lock, "wait_queue");
    INIT_LIST_HEAD(&wq_head->head);
}

/* Initialize a wait queue entry */
static inline void wait_queue_entry_init(wait_queue_entry_t *wq_entry, int flags, void *private, int (*func)(struct wait_queue_entry *, unsigned, int, void *)) {
    wq_entry->flags = flags;
    wq_entry->private = private;
    wq_entry->func = func;
    INIT_LIST_HEAD(&wq_entry->link);
}

/* Add a wait queue entry to a wait queue */
void wait_queue_add(wait_queue_head_t *wq_head, wait_queue_entry_t *wq_entry);

/* Remove a wait queue entry from a wait queue */
void wait_queue_remove(wait_queue_head_t *wq_head, wait_queue_entry_t *wq_entry);

/* Wake up a wait queue */
void wake_up(wait_queue_head_t *wq_head);

/* Wake up all waiters in a wait queue */
void wake_up_all(wait_queue_head_t *wq_head);

/* Wake up interruptible waiters in a wait queue */
void wake_up_interruptible(wait_queue_head_t *wq_head);

/* Wake up all interruptible waiters in a wait queue */
void wake_up_interruptible_all(wait_queue_head_t *wq_head);

/* Wait on a wait queue */
void wait_event(wait_queue_head_t *wq_head, int condition);

/* Wait on a wait queue with timeout */
int wait_event_timeout(wait_queue_head_t *wq_head, int condition, unsigned long timeout);

/* Wait on a wait queue (interruptible) */
int wait_event_interruptible(wait_queue_head_t *wq_head, int condition);

/* Wait on a wait queue with timeout (interruptible) */
int wait_event_interruptible_timeout(wait_queue_head_t *wq_head, int condition, unsigned long timeout);

#endif /* _HORIZON_WAIT_H */
