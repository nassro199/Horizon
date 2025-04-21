/**
 * thread_sync.c - Horizon kernel thread synchronization implementation
 * 
 * This file contains the implementation of thread synchronization primitives.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/task.h>
#include <horizon/thread.h>
#include <horizon/sync.h>
#include <horizon/sched.h>
#include <horizon/mm.h>
#include <horizon/spinlock.h>
#include <horizon/printk.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/**
 * Initialize a mutex
 * 
 * @param mutex Mutex to initialize
 * @return 0 on success, negative error code on failure
 */
int mutex_init(mutex_t *mutex) {
    /* Check parameters */
    if (mutex == NULL) {
        return -EINVAL;
    }
    
    /* Initialize the mutex */
    mutex->locked = 0;
    mutex->owner = NULL;
    list_init(&mutex->waiters);
    
    return 0;
}

/**
 * Destroy a mutex
 * 
 * @param mutex Mutex to destroy
 * @return 0 on success, negative error code on failure
 */
int mutex_destroy(mutex_t *mutex) {
    /* Check parameters */
    if (mutex == NULL) {
        return -EINVAL;
    }
    
    /* Check if the mutex is locked */
    if (mutex->locked) {
        return -EBUSY;
    }
    
    /* Check if there are waiters */
    if (!list_empty(&mutex->waiters)) {
        return -EBUSY;
    }
    
    return 0;
}

/**
 * Lock a mutex
 * 
 * @param mutex Mutex to lock
 * @return 0 on success, negative error code on failure
 */
int mutex_lock(mutex_t *mutex) {
    /* Check parameters */
    if (mutex == NULL) {
        return -EINVAL;
    }
    
    /* Get the current thread */
    thread_t *thread = thread_self();
    
    /* Try to lock the mutex */
    if (__sync_bool_compare_and_swap(&mutex->locked, 0, 1)) {
        /* Mutex was unlocked, now it's locked */
        mutex->owner = thread;
        return 0;
    }
    
    /* Check if the mutex is already owned by this thread */
    if (mutex->owner == thread) {
        return -EDEADLK;
    }
    
    /* Add the thread to the waiters list */
    list_add_tail(&thread->wait_list, &mutex->waiters);
    
    /* Block the thread */
    sched_block_thread(thread);
    
    /* When the thread is woken up, the mutex is locked */
    return 0;
}

/**
 * Try to lock a mutex
 * 
 * @param mutex Mutex to lock
 * @return 0 on success, negative error code on failure
 */
int mutex_trylock(mutex_t *mutex) {
    /* Check parameters */
    if (mutex == NULL) {
        return -EINVAL;
    }
    
    /* Get the current thread */
    thread_t *thread = thread_self();
    
    /* Try to lock the mutex */
    if (__sync_bool_compare_and_swap(&mutex->locked, 0, 1)) {
        /* Mutex was unlocked, now it's locked */
        mutex->owner = thread;
        return 0;
    }
    
    /* Mutex is already locked */
    return -EBUSY;
}

/**
 * Unlock a mutex
 * 
 * @param mutex Mutex to unlock
 * @return 0 on success, negative error code on failure
 */
int mutex_unlock(mutex_t *mutex) {
    /* Check parameters */
    if (mutex == NULL) {
        return -EINVAL;
    }
    
    /* Get the current thread */
    thread_t *thread = thread_self();
    
    /* Check if the mutex is owned by this thread */
    if (mutex->owner != thread) {
        return -EPERM;
    }
    
    /* Check if there are waiters */
    if (!list_empty(&mutex->waiters)) {
        /* Get the first waiter */
        thread_t *waiter = list_first_entry(&mutex->waiters, thread_t, wait_list);
        
        /* Remove the waiter from the list */
        list_del(&waiter->wait_list);
        
        /* Set the new owner */
        mutex->owner = waiter;
        
        /* Wake up the waiter */
        sched_unblock_thread(waiter);
    } else {
        /* No waiters, just unlock the mutex */
        mutex->owner = NULL;
        mutex->locked = 0;
    }
    
    return 0;
}

/**
 * Initialize a semaphore
 * 
 * @param sem Semaphore to initialize
 * @param value Initial value
 * @return 0 on success, negative error code on failure
 */
int sem_init(sem_t *sem, u32 value) {
    /* Check parameters */
    if (sem == NULL) {
        return -EINVAL;
    }
    
    /* Initialize the semaphore */
    sem->value = value;
    list_init(&sem->waiters);
    
    return 0;
}

/**
 * Destroy a semaphore
 * 
 * @param sem Semaphore to destroy
 * @return 0 on success, negative error code on failure
 */
int sem_destroy(sem_t *sem) {
    /* Check parameters */
    if (sem == NULL) {
        return -EINVAL;
    }
    
    /* Check if there are waiters */
    if (!list_empty(&sem->waiters)) {
        return -EBUSY;
    }
    
    return 0;
}

/**
 * Wait on a semaphore
 * 
 * @param sem Semaphore to wait on
 * @return 0 on success, negative error code on failure
 */
int sem_wait(sem_t *sem) {
    /* Check parameters */
    if (sem == NULL) {
        return -EINVAL;
    }
    
    /* Get the current thread */
    thread_t *thread = thread_self();
    
    /* Decrement the semaphore value */
    if (__sync_fetch_and_sub(&sem->value, 1) <= 0) {
        /* Value is now negative, block the thread */
        list_add_tail(&thread->wait_list, &sem->waiters);
        sched_block_thread(thread);
    }
    
    return 0;
}

/**
 * Try to wait on a semaphore
 * 
 * @param sem Semaphore to wait on
 * @return 0 on success, negative error code on failure
 */
int sem_trywait(sem_t *sem) {
    /* Check parameters */
    if (sem == NULL) {
        return -EINVAL;
    }
    
    /* Try to decrement the semaphore value */
    if (sem->value > 0 && __sync_fetch_and_sub(&sem->value, 1) > 0) {
        /* Value was positive, now it's one less */
        return 0;
    }
    
    /* Value is zero or negative */
    return -EAGAIN;
}

/**
 * Post to a semaphore
 * 
 * @param sem Semaphore to post to
 * @return 0 on success, negative error code on failure
 */
int sem_post(sem_t *sem) {
    /* Check parameters */
    if (sem == NULL) {
        return -EINVAL;
    }
    
    /* Increment the semaphore value */
    if (__sync_fetch_and_add(&sem->value, 1) < 0) {
        /* Value was negative, wake up a waiter */
        if (!list_empty(&sem->waiters)) {
            /* Get the first waiter */
            thread_t *waiter = list_first_entry(&sem->waiters, thread_t, wait_list);
            
            /* Remove the waiter from the list */
            list_del(&waiter->wait_list);
            
            /* Wake up the waiter */
            sched_unblock_thread(waiter);
        }
    }
    
    return 0;
}
