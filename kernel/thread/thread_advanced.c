/**
 * thread_advanced.c - Horizon kernel advanced thread management implementation
 * 
 * This file contains the implementation of advanced thread management features.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/task.h>
#include <horizon/thread.h>
#include <horizon/sched.h>
#include <horizon/mm.h>
#include <horizon/spinlock.h>
#include <horizon/printk.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Thread statistics */
static u64 thread_create_count = 0;
static u64 thread_exit_count = 0;
static u64 thread_join_count = 0;
static u64 thread_detach_count = 0;
static u64 thread_cancel_count = 0;

/* Thread lock */
static spinlock_t thread_lock = SPIN_LOCK_INITIALIZER;

/**
 * Set thread priority
 * 
 * @param thread Thread to set priority for
 * @param priority Priority to set
 * @return 0 on success, negative error code on failure
 */
int thread_set_priority(thread_t *thread, int priority) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Set the thread's priority */
    thread->priority = priority;
    
    /* Update the scheduler */
    sched_set_priority(thread, priority);
    
    return 0;
}

/**
 * Set thread affinity
 * 
 * @param thread Thread to set affinity for
 * @param cpu CPU to set
 * @return 0 on success, negative error code on failure
 */
int thread_set_affinity(thread_t *thread, u32 cpu) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Set the thread's CPU */
    thread->cpu = cpu;
    
    /* Update the scheduler */
    sched_set_affinity(thread, cpu);
    
    return 0;
}

/**
 * Set thread name
 * 
 * @param thread Thread to set name for
 * @param name Name to set
 * @return 0 on success, negative error code on failure
 */
int thread_set_name(thread_t *thread, const char *name) {
    /* Check parameters */
    if (thread == NULL || name == NULL) {
        return -EINVAL;
    }
    
    /* Set the thread's name */
    strncpy(thread->name, name, THREAD_NAME_MAX - 1);
    thread->name[THREAD_NAME_MAX - 1] = '\0';
    
    return 0;
}

/**
 * Get thread statistics
 * 
 * @param thread Thread to get statistics for
 * @param stats Statistics structure to fill
 * @return 0 on success, negative error code on failure
 */
int thread_get_stats(thread_t *thread, thread_stats_t *stats) {
    /* Check parameters */
    if (thread == NULL || stats == NULL) {
        return -EINVAL;
    }
    
    /* Fill the statistics */
    stats->tid = thread->tid;
    stats->state = thread->state;
    stats->priority = thread->priority;
    stats->cpu = thread->cpu;
    stats->time_slice = thread->time_slice;
    stats->runtime = thread->runtime;
    stats->switches = thread->switches;
    
    return 0;
}

/**
 * Yield the current thread
 */
void thread_yield(void) {
    /* Yield the CPU */
    sched_yield();
}

/**
 * Sleep the current thread
 * 
 * @param ms Milliseconds to sleep
 */
void thread_sleep(u32 ms) {
    /* Sleep the thread */
    sched_sleep_thread(current_thread(), ms);
}

/**
 * Wake up a thread
 * 
 * @param thread Thread to wake up
 * @return 0 on success, negative error code on failure
 */
int thread_wakeup(thread_t *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Wake up the thread */
    sched_wakeup_thread(thread);
    
    return 0;
}

/**
 * Print thread statistics
 */
void thread_print_stats(void) {
    /* Print the statistics */
    printk(KERN_INFO "THREAD: Create count: %llu\n", thread_create_count);
    printk(KERN_INFO "THREAD: Exit count: %llu\n", thread_exit_count);
    printk(KERN_INFO "THREAD: Join count: %llu\n", thread_join_count);
    printk(KERN_INFO "THREAD: Detach count: %llu\n", thread_detach_count);
    printk(KERN_INFO "THREAD: Cancel count: %llu\n", thread_cancel_count);
}
