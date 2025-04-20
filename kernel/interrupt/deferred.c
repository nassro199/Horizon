/**
 * deferred.c - Deferred interrupt processing
 * 
 * This file contains the implementation of deferred interrupt processing.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/interrupt.h>
#include <horizon/spinlock.h>
#include <horizon/errno.h>

/* Deferred work item */
typedef struct deferred_work {
    void (*func)(void *data);      /* Function to call */
    void *data;                    /* Data to pass to function */
    struct deferred_work *next;    /* Next work item */
} deferred_work_t;

/* Deferred work queue */
static deferred_work_t *deferred_work_queue = NULL;
static spinlock_t deferred_work_lock = SPIN_LOCK_INITIALIZER;

/**
 * Add deferred work
 * 
 * @param func Function to call
 * @param data Data to pass to function
 * @return 0 on success, negative error code on failure
 */
int interrupt_defer_work(void (*func)(void *data), void *data) {
    deferred_work_t *work;
    
    /* Check parameters */
    if (func == NULL) {
        return -EINVAL;
    }
    
    /* Allocate work item */
    work = kmalloc(sizeof(deferred_work_t), MEM_KERNEL);
    if (work == NULL) {
        return -ENOMEM;
    }
    
    /* Initialize work item */
    work->func = func;
    work->data = data;
    
    /* Add to queue */
    spin_lock(&deferred_work_lock);
    work->next = deferred_work_queue;
    deferred_work_queue = work;
    spin_unlock(&deferred_work_lock);
    
    return 0;
}

/**
 * Process deferred work
 */
void check_deferred_work(void) {
    deferred_work_t *work, *next;
    
    /* Get the work queue */
    spin_lock(&deferred_work_lock);
    work = deferred_work_queue;
    deferred_work_queue = NULL;
    spin_unlock(&deferred_work_lock);
    
    /* Process work items */
    while (work != NULL) {
        next = work->next;
        work->func(work->data);
        kfree(work);
        work = next;
    }
}
