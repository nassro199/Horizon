/**
 * wait.c - Horizon kernel wait queue implementation
 * 
 * This file contains the implementation of wait queues.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/wait.h>
#include <horizon/sched.h>
#include <horizon/spinlock.h>
#include <horizon/list.h>

/* Default wake function */
static int default_wake_function(wait_queue_entry_t *wq_entry, unsigned mode, int flags, void *key) {
    /* Wake up the task */
    task_struct_t *task = (task_struct_t *)wq_entry->private;
    return task_wakeup(task);
}

/**
 * Add a wait queue entry to a wait queue
 * 
 * @param wq_head Wait queue head
 * @param wq_entry Wait queue entry
 */
void wait_queue_add(wait_queue_head_t *wq_head, wait_queue_entry_t *wq_entry) {
    /* Check parameters */
    if (wq_head == NULL || wq_entry == NULL) {
        return;
    }
    
    /* Add to wait queue */
    spin_lock(&wq_head->lock);
    if (list_empty(&wq_entry->link)) {
        if (wq_entry->flags & WQ_FLAG_EXCLUSIVE) {
            /* Add to end of queue */
            list_add_tail(&wq_entry->link, &wq_head->head);
        } else {
            /* Add to beginning of queue */
            list_add(&wq_entry->link, &wq_head->head);
        }
    }
    spin_unlock(&wq_head->lock);
}

/**
 * Remove a wait queue entry from a wait queue
 * 
 * @param wq_head Wait queue head
 * @param wq_entry Wait queue entry
 */
void wait_queue_remove(wait_queue_head_t *wq_head, wait_queue_entry_t *wq_entry) {
    /* Check parameters */
    if (wq_head == NULL || wq_entry == NULL) {
        return;
    }
    
    /* Remove from wait queue */
    spin_lock(&wq_head->lock);
    if (!list_empty(&wq_entry->link)) {
        list_del_init(&wq_entry->link);
    }
    spin_unlock(&wq_head->lock);
}

/**
 * Wake up a wait queue
 * 
 * @param wq_head Wait queue head
 */
void wake_up(wait_queue_head_t *wq_head) {
    /* Check parameters */
    if (wq_head == NULL) {
        return;
    }
    
    /* Wake up waiters */
    spin_lock(&wq_head->lock);
    if (!list_empty(&wq_head->head)) {
        wait_queue_entry_t *wq_entry;
        list_for_each_entry(wq_entry, &wq_head->head, link) {
            if (wq_entry->func(wq_entry, 0, 0, NULL)) {
                break;
            }
        }
    }
    spin_unlock(&wq_head->lock);
}

/**
 * Wake up all waiters in a wait queue
 * 
 * @param wq_head Wait queue head
 */
void wake_up_all(wait_queue_head_t *wq_head) {
    /* Check parameters */
    if (wq_head == NULL) {
        return;
    }
    
    /* Wake up all waiters */
    spin_lock(&wq_head->lock);
    if (!list_empty(&wq_head->head)) {
        wait_queue_entry_t *wq_entry;
        list_for_each_entry(wq_entry, &wq_head->head, link) {
            wq_entry->func(wq_entry, 0, 0, NULL);
        }
    }
    spin_unlock(&wq_head->lock);
}

/**
 * Wake up interruptible waiters in a wait queue
 * 
 * @param wq_head Wait queue head
 */
void wake_up_interruptible(wait_queue_head_t *wq_head) {
    /* Check parameters */
    if (wq_head == NULL) {
        return;
    }
    
    /* Wake up interruptible waiters */
    spin_lock(&wq_head->lock);
    if (!list_empty(&wq_head->head)) {
        wait_queue_entry_t *wq_entry;
        list_for_each_entry(wq_entry, &wq_head->head, link) {
            if (!(wq_entry->flags & WQ_FLAG_EXCLUSIVE) && wq_entry->func(wq_entry, 0, 0, NULL)) {
                break;
            }
        }
    }
    spin_unlock(&wq_head->lock);
}

/**
 * Wake up all interruptible waiters in a wait queue
 * 
 * @param wq_head Wait queue head
 */
void wake_up_interruptible_all(wait_queue_head_t *wq_head) {
    /* Check parameters */
    if (wq_head == NULL) {
        return;
    }
    
    /* Wake up all interruptible waiters */
    spin_lock(&wq_head->lock);
    if (!list_empty(&wq_head->head)) {
        wait_queue_entry_t *wq_entry;
        list_for_each_entry(wq_entry, &wq_head->head, link) {
            if (!(wq_entry->flags & WQ_FLAG_EXCLUSIVE)) {
                wq_entry->func(wq_entry, 0, 0, NULL);
            }
        }
    }
    spin_unlock(&wq_head->lock);
}

/**
 * Wait on a wait queue
 * 
 * @param wq_head Wait queue head
 * @param condition Condition to wait for
 */
void wait_event(wait_queue_head_t *wq_head, int condition) {
    /* Check parameters */
    if (wq_head == NULL) {
        return;
    }
    
    /* Check condition */
    if (condition) {
        return;
    }
    
    /* Create wait queue entry */
    wait_queue_entry_t wq_entry;
    wait_queue_entry_init(&wq_entry, 0, current, default_wake_function);
    
    /* Add to wait queue */
    wait_queue_add(wq_head, &wq_entry);
    
    /* Wait until condition is true */
    while (!condition) {
        /* Set task state to uninterruptible */
        current->state = TASK_UNINTERRUPTIBLE;
        
        /* Schedule another task */
        schedule();
    }
    
    /* Remove from wait queue */
    wait_queue_remove(wq_head, &wq_entry);
}

/**
 * Wait on a wait queue with timeout
 * 
 * @param wq_head Wait queue head
 * @param condition Condition to wait for
 * @param timeout Timeout in jiffies
 * @return Remaining jiffies, or 0 if timed out
 */
int wait_event_timeout(wait_queue_head_t *wq_head, int condition, unsigned long timeout) {
    /* Check parameters */
    if (wq_head == NULL || timeout == 0) {
        return 0;
    }
    
    /* Check condition */
    if (condition) {
        return timeout;
    }
    
    /* Create wait queue entry */
    wait_queue_entry_t wq_entry;
    wait_queue_entry_init(&wq_entry, 0, current, default_wake_function);
    
    /* Add to wait queue */
    wait_queue_add(wq_head, &wq_entry);
    
    /* Wait until condition is true or timeout */
    unsigned long expire = jiffies + timeout;
    while (!condition && time_before(jiffies, expire)) {
        /* Set task state to uninterruptible */
        current->state = TASK_UNINTERRUPTIBLE;
        
        /* Schedule another task */
        schedule_timeout(expire - jiffies);
    }
    
    /* Remove from wait queue */
    wait_queue_remove(wq_head, &wq_entry);
    
    /* Return remaining time */
    if (condition) {
        return expire - jiffies;
    }
    
    return 0;
}

/**
 * Wait on a wait queue (interruptible)
 * 
 * @param wq_head Wait queue head
 * @param condition Condition to wait for
 * @return 0 if condition is true, -ERESTARTSYS if interrupted
 */
int wait_event_interruptible(wait_queue_head_t *wq_head, int condition) {
    /* Check parameters */
    if (wq_head == NULL) {
        return -EINVAL;
    }
    
    /* Check condition */
    if (condition) {
        return 0;
    }
    
    /* Create wait queue entry */
    wait_queue_entry_t wq_entry;
    wait_queue_entry_init(&wq_entry, 0, current, default_wake_function);
    
    /* Add to wait queue */
    wait_queue_add(wq_head, &wq_entry);
    
    /* Wait until condition is true or interrupted */
    while (!condition) {
        /* Set task state to interruptible */
        current->state = TASK_INTERRUPTIBLE;
        
        /* Schedule another task */
        schedule();
        
        /* Check if interrupted */
        if (signal_pending(current)) {
            wait_queue_remove(wq_head, &wq_entry);
            return -ERESTARTSYS;
        }
    }
    
    /* Remove from wait queue */
    wait_queue_remove(wq_head, &wq_entry);
    
    return 0;
}

/**
 * Wait on a wait queue with timeout (interruptible)
 * 
 * @param wq_head Wait queue head
 * @param condition Condition to wait for
 * @param timeout Timeout in jiffies
 * @return Remaining jiffies, 0 if timed out, or -ERESTARTSYS if interrupted
 */
int wait_event_interruptible_timeout(wait_queue_head_t *wq_head, int condition, unsigned long timeout) {
    /* Check parameters */
    if (wq_head == NULL || timeout == 0) {
        return 0;
    }
    
    /* Check condition */
    if (condition) {
        return timeout;
    }
    
    /* Create wait queue entry */
    wait_queue_entry_t wq_entry;
    wait_queue_entry_init(&wq_entry, 0, current, default_wake_function);
    
    /* Add to wait queue */
    wait_queue_add(wq_head, &wq_entry);
    
    /* Wait until condition is true, timeout, or interrupted */
    unsigned long expire = jiffies + timeout;
    while (!condition && time_before(jiffies, expire)) {
        /* Set task state to interruptible */
        current->state = TASK_INTERRUPTIBLE;
        
        /* Schedule another task */
        schedule_timeout(expire - jiffies);
        
        /* Check if interrupted */
        if (signal_pending(current)) {
            wait_queue_remove(wq_head, &wq_entry);
            return -ERESTARTSYS;
        }
    }
    
    /* Remove from wait queue */
    wait_queue_remove(wq_head, &wq_entry);
    
    /* Return remaining time */
    if (condition) {
        return expire - jiffies;
    }
    
    return 0;
}
