/**
 * rt.c - Horizon kernel real-time scheduler implementation
 * 
 * This file contains the implementation of real-time scheduling policies.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/sched.h>
#include <horizon/sched/rt.h>
#include <horizon/thread.h>
#include <horizon/task.h>
#include <horizon/mm.h>
#include <horizon/spinlock.h>
#include <horizon/printk.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Real-time statistics */
static u64 rt_schedule_count = 0;
static u64 rt_switch_count = 0;
static u64 rt_preempt_count = 0;
static u64 rt_yield_count = 0;
static u64 rt_boost_count = 0;
static u64 rt_throttle_count = 0;

/* Real-time lock */
static spinlock_t rt_lock = SPIN_LOCK_INITIALIZER;

/* Real-time parameters */
static int rt_enabled = 1;
static u64 rt_runtime = 950000; /* 95% of CPU time */
static u64 rt_period = 1000000; /* 1 second */
static u32 rt_prio_base = 100;  /* Base priority for real-time threads */

/**
 * Initialize the real-time scheduler
 */
void rt_init(void) {
    /* Reset statistics */
    rt_schedule_count = 0;
    rt_switch_count = 0;
    rt_preempt_count = 0;
    rt_yield_count = 0;
    rt_boost_count = 0;
    rt_throttle_count = 0;
    
    /* Set parameters */
    rt_enabled = 1;
    rt_runtime = 950000; /* 95% of CPU time */
    rt_period = 1000000; /* 1 second */
    rt_prio_base = 100;  /* Base priority for real-time threads */
    
    printk(KERN_INFO "RT: Initialized real-time scheduler\n");
}

/**
 * Enable or disable real-time scheduling
 * 
 * @param enable 1 to enable, 0 to disable
 * @return 0 on success, negative error code on failure
 */
int rt_enable(int enable) {
    /* Lock the real-time scheduler */
    spin_lock(&rt_lock);
    
    /* Set the state */
    rt_enabled = enable ? 1 : 0;
    
    /* Unlock the real-time scheduler */
    spin_unlock(&rt_lock);
    
    printk(KERN_INFO "RT: %s real-time scheduling\n", enable ? "Enabled" : "Disabled");
    
    return 0;
}

/**
 * Set the real-time runtime
 * 
 * @param runtime Runtime in microseconds
 * @return 0 on success, negative error code on failure
 */
int rt_set_runtime(u64 runtime) {
    /* Check parameters */
    if (runtime > rt_period) {
        return -EINVAL;
    }
    
    /* Lock the real-time scheduler */
    spin_lock(&rt_lock);
    
    /* Set the runtime */
    rt_runtime = runtime;
    
    /* Unlock the real-time scheduler */
    spin_unlock(&rt_lock);
    
    printk(KERN_INFO "RT: Set runtime to %llu us\n", runtime);
    
    return 0;
}

/**
 * Set the real-time period
 * 
 * @param period Period in microseconds
 * @return 0 on success, negative error code on failure
 */
int rt_set_period(u64 period) {
    /* Check parameters */
    if (period == 0 || rt_runtime > period) {
        return -EINVAL;
    }
    
    /* Lock the real-time scheduler */
    spin_lock(&rt_lock);
    
    /* Set the period */
    rt_period = period;
    
    /* Unlock the real-time scheduler */
    spin_unlock(&rt_lock);
    
    printk(KERN_INFO "RT: Set period to %llu us\n", period);
    
    return 0;
}

/**
 * Set the real-time priority base
 * 
 * @param prio_base Priority base
 * @return 0 on success, negative error code on failure
 */
int rt_set_prio_base(u32 prio_base) {
    /* Lock the real-time scheduler */
    spin_lock(&rt_lock);
    
    /* Set the priority base */
    rt_prio_base = prio_base;
    
    /* Unlock the real-time scheduler */
    spin_unlock(&rt_lock);
    
    printk(KERN_INFO "RT: Set priority base to %u\n", prio_base);
    
    return 0;
}

/**
 * Check if a thread is real-time
 * 
 * @param thread Thread to check
 * @return 1 if the thread is real-time, 0 if not
 */
int rt_is_realtime(thread_t *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return 0;
    }
    
    /* Check if real-time scheduling is enabled */
    if (!rt_enabled) {
        return 0;
    }
    
    /* Check if the thread has a real-time policy */
    return thread->policy == SCHED_FIFO || thread->policy == SCHED_RR;
}

/**
 * Check if a thread can preempt another thread
 * 
 * @param thread Thread to check
 * @param current Current thread
 * @return 1 if the thread can preempt the current thread, 0 if not
 */
int rt_can_preempt(thread_t *thread, thread_t *current) {
    /* Check parameters */
    if (thread == NULL || current == NULL) {
        return 0;
    }
    
    /* Check if real-time scheduling is enabled */
    if (!rt_enabled) {
        return 0;
    }
    
    /* Check if the thread is real-time */
    if (!rt_is_realtime(thread)) {
        return 0;
    }
    
    /* Check if the current thread is real-time */
    if (rt_is_realtime(current)) {
        /* Both threads are real-time, check priorities */
        return thread->priority < current->priority;
    } else {
        /* The thread is real-time, the current thread is not */
        return 1;
    }
}

/**
 * Schedule a real-time thread
 * 
 * @param rq Run queue to schedule on
 * @return Pointer to the next thread to run, or NULL if none
 */
thread_t *rt_schedule(run_queue_t *rq) {
    /* Check parameters */
    if (rq == NULL) {
        return NULL;
    }
    
    /* Check if real-time scheduling is enabled */
    if (!rt_enabled) {
        return NULL;
    }
    
    /* Lock the real-time scheduler */
    spin_lock(&rt_lock);
    
    /* Increment the schedule count */
    rt_schedule_count++;
    
    /* Find the highest priority real-time thread */
    thread_t *next = NULL;
    
    for (int i = 0; i <= SCHED_PRIO_MAX; i++) {
        /* Check if there are threads at this priority */
        if (list_empty(&rq->active[i])) {
            continue;
        }
        
        /* Get the first thread at this priority */
        thread_t *thread = list_first_entry(&rq->active[i], thread_t, sched_list);
        
        /* Check if the thread is real-time */
        if (rt_is_realtime(thread)) {
            /* Found a real-time thread */
            next = thread;
            break;
        }
    }
    
    /* Unlock the real-time scheduler */
    spin_unlock(&rt_lock);
    
    return next;
}

/**
 * Enqueue a real-time thread
 * 
 * @param rq Run queue to enqueue on
 * @param thread Thread to enqueue
 * @return 0 on success, negative error code on failure
 */
int rt_enqueue(run_queue_t *rq, thread_t *thread) {
    /* Check parameters */
    if (rq == NULL || thread == NULL) {
        return -EINVAL;
    }
    
    /* Check if real-time scheduling is enabled */
    if (!rt_enabled) {
        return -EINVAL;
    }
    
    /* Check if the thread is real-time */
    if (!rt_is_realtime(thread)) {
        return -EINVAL;
    }
    
    /* Lock the real-time scheduler */
    spin_lock(&rt_lock);
    
    /* Get the thread's priority */
    int priority = thread->priority;
    
    /* Add the thread to the active array */
    list_add_tail(&thread->sched_list, &rq->active[priority]);
    
    /* Set the bit in the bitmap */
    rq->bitmap |= (1ULL << priority);
    
    /* Unlock the real-time scheduler */
    spin_unlock(&rt_lock);
    
    return 0;
}

/**
 * Dequeue a real-time thread
 * 
 * @param rq Run queue to dequeue from
 * @param thread Thread to dequeue
 * @return 0 on success, negative error code on failure
 */
int rt_dequeue(run_queue_t *rq, thread_t *thread) {
    /* Check parameters */
    if (rq == NULL || thread == NULL) {
        return -EINVAL;
    }
    
    /* Check if real-time scheduling is enabled */
    if (!rt_enabled) {
        return -EINVAL;
    }
    
    /* Check if the thread is real-time */
    if (!rt_is_realtime(thread)) {
        return -EINVAL;
    }
    
    /* Lock the real-time scheduler */
    spin_lock(&rt_lock);
    
    /* Get the thread's priority */
    int priority = thread->priority;
    
    /* Remove the thread from the active array */
    list_del(&thread->sched_list);
    
    /* Check if there are any more threads at this priority */
    if (list_empty(&rq->active[priority])) {
        /* Clear the bit in the bitmap */
        rq->bitmap &= ~(1ULL << priority);
    }
    
    /* Unlock the real-time scheduler */
    spin_unlock(&rt_lock);
    
    return 0;
}

/**
 * Yield a real-time thread
 * 
 * @param rq Run queue to yield on
 * @param thread Thread to yield
 * @return 0 on success, negative error code on failure
 */
int rt_yield(run_queue_t *rq, thread_t *thread) {
    /* Check parameters */
    if (rq == NULL || thread == NULL) {
        return -EINVAL;
    }
    
    /* Check if real-time scheduling is enabled */
    if (!rt_enabled) {
        return -EINVAL;
    }
    
    /* Check if the thread is real-time */
    if (!rt_is_realtime(thread)) {
        return -EINVAL;
    }
    
    /* Lock the real-time scheduler */
    spin_lock(&rt_lock);
    
    /* Increment the yield count */
    rt_yield_count++;
    
    /* Get the thread's priority */
    int priority = thread->priority;
    
    /* Check if there are other threads at this priority */
    if (!list_empty(&rq->active[priority]) && !list_is_singular(&rq->active[priority])) {
        /* Remove the thread from the active array */
        list_del(&thread->sched_list);
        
        /* Add the thread to the end of the active array */
        list_add_tail(&thread->sched_list, &rq->active[priority]);
    }
    
    /* Unlock the real-time scheduler */
    spin_unlock(&rt_lock);
    
    return 0;
}

/**
 * Boost a real-time thread's priority
 * 
 * @param thread Thread to boost
 * @param boost Boost amount
 * @return 0 on success, negative error code on failure
 */
int rt_boost(thread_t *thread, int boost) {
    /* Check parameters */
    if (thread == NULL || boost <= 0) {
        return -EINVAL;
    }
    
    /* Check if real-time scheduling is enabled */
    if (!rt_enabled) {
        return -EINVAL;
    }
    
    /* Check if the thread is real-time */
    if (!rt_is_realtime(thread)) {
        return -EINVAL;
    }
    
    /* Lock the real-time scheduler */
    spin_lock(&rt_lock);
    
    /* Increment the boost count */
    rt_boost_count++;
    
    /* Get the thread's run queue */
    run_queue_t *rq = thread->rq;
    
    /* Get the thread's priority */
    int priority = thread->priority;
    
    /* Calculate the new priority */
    int new_priority = priority - boost;
    
    /* Clamp the priority */
    if (new_priority < 0) {
        new_priority = 0;
    }
    
    /* Check if the priority changed */
    if (new_priority != priority) {
        /* Remove the thread from the active array */
        list_del(&thread->sched_list);
        
        /* Check if there are any more threads at this priority */
        if (list_empty(&rq->active[priority])) {
            /* Clear the bit in the bitmap */
            rq->bitmap &= ~(1ULL << priority);
        }
        
        /* Set the thread's priority */
        thread->priority = new_priority;
        
        /* Add the thread to the active array */
        list_add_tail(&thread->sched_list, &rq->active[new_priority]);
        
        /* Set the bit in the bitmap */
        rq->bitmap |= (1ULL << new_priority);
    }
    
    /* Unlock the real-time scheduler */
    spin_unlock(&rt_lock);
    
    return 0;
}

/**
 * Throttle a real-time thread
 * 
 * @param thread Thread to throttle
 * @param throttle Throttle amount
 * @return 0 on success, negative error code on failure
 */
int rt_throttle(thread_t *thread, int throttle) {
    /* Check parameters */
    if (thread == NULL || throttle <= 0) {
        return -EINVAL;
    }
    
    /* Check if real-time scheduling is enabled */
    if (!rt_enabled) {
        return -EINVAL;
    }
    
    /* Check if the thread is real-time */
    if (!rt_is_realtime(thread)) {
        return -EINVAL;
    }
    
    /* Lock the real-time scheduler */
    spin_lock(&rt_lock);
    
    /* Increment the throttle count */
    rt_throttle_count++;
    
    /* Get the thread's run queue */
    run_queue_t *rq = thread->rq;
    
    /* Get the thread's priority */
    int priority = thread->priority;
    
    /* Calculate the new priority */
    int new_priority = priority + throttle;
    
    /* Clamp the priority */
    if (new_priority > SCHED_PRIO_MAX) {
        new_priority = SCHED_PRIO_MAX;
    }
    
    /* Check if the priority changed */
    if (new_priority != priority) {
        /* Remove the thread from the active array */
        list_del(&thread->sched_list);
        
        /* Check if there are any more threads at this priority */
        if (list_empty(&rq->active[priority])) {
            /* Clear the bit in the bitmap */
            rq->bitmap &= ~(1ULL << priority);
        }
        
        /* Set the thread's priority */
        thread->priority = new_priority;
        
        /* Add the thread to the active array */
        list_add_tail(&thread->sched_list, &rq->active[new_priority]);
        
        /* Set the bit in the bitmap */
        rq->bitmap |= (1ULL << new_priority);
    }
    
    /* Unlock the real-time scheduler */
    spin_unlock(&rt_lock);
    
    return 0;
}

/**
 * Print real-time scheduler statistics
 */
void rt_print_stats(void) {
    /* Lock the real-time scheduler */
    spin_lock(&rt_lock);
    
    /* Print the statistics */
    printk(KERN_INFO "RT: Enabled: %s\n", rt_enabled ? "Yes" : "No");
    printk(KERN_INFO "RT: Runtime: %llu us\n", rt_runtime);
    printk(KERN_INFO "RT: Period: %llu us\n", rt_period);
    printk(KERN_INFO "RT: Priority base: %u\n", rt_prio_base);
    printk(KERN_INFO "RT: Schedule count: %llu\n", rt_schedule_count);
    printk(KERN_INFO "RT: Switch count: %llu\n", rt_switch_count);
    printk(KERN_INFO "RT: Preempt count: %llu\n", rt_preempt_count);
    printk(KERN_INFO "RT: Yield count: %llu\n", rt_yield_count);
    printk(KERN_INFO "RT: Boost count: %llu\n", rt_boost_count);
    printk(KERN_INFO "RT: Throttle count: %llu\n", rt_throttle_count);
    
    /* Unlock the real-time scheduler */
    spin_unlock(&rt_lock);
}
