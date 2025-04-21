/**
 * load_balance.c - Horizon kernel load balancing implementation
 * 
 * This file contains the implementation of CPU load balancing.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/sched.h>
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

/* Load balancing statistics */
static u64 load_balance_count = 0;
static u64 load_balance_moves = 0;
static u64 load_balance_failed = 0;
static u64 load_balance_skipped = 0;
static u64 load_balance_imbalance = 0;

/* Load balancing lock */
static spinlock_t load_balance_lock = SPIN_LOCK_INITIALIZER;

/* Load balancing parameters */
static int load_balance_enabled = 1;
static u64 load_balance_interval = 1000; /* 1 second */
static u64 load_balance_threshold = 25; /* 25% imbalance */
static u64 load_balance_last_time = 0;

/**
 * Initialize the load balancing subsystem
 */
void load_balance_init(void) {
    /* Reset statistics */
    load_balance_count = 0;
    load_balance_moves = 0;
    load_balance_failed = 0;
    load_balance_skipped = 0;
    load_balance_imbalance = 0;
    
    /* Set parameters */
    load_balance_enabled = 1;
    load_balance_interval = 1000; /* 1 second */
    load_balance_threshold = 25; /* 25% imbalance */
    load_balance_last_time = 0;
    
    printk(KERN_INFO "LOAD_BALANCE: Initialized load balancing subsystem\n");
}

/**
 * Enable or disable load balancing
 * 
 * @param enable 1 to enable, 0 to disable
 * @return 0 on success, negative error code on failure
 */
int load_balance_enable(int enable) {
    /* Lock the load balancing */
    spin_lock(&load_balance_lock);
    
    /* Set the state */
    load_balance_enabled = enable ? 1 : 0;
    
    /* Unlock the load balancing */
    spin_unlock(&load_balance_lock);
    
    printk(KERN_INFO "LOAD_BALANCE: %s load balancing\n", enable ? "Enabled" : "Disabled");
    
    return 0;
}

/**
 * Set the load balancing interval
 * 
 * @param interval Interval in milliseconds
 * @return 0 on success, negative error code on failure
 */
int load_balance_set_interval(u64 interval) {
    /* Check parameters */
    if (interval == 0) {
        return -EINVAL;
    }
    
    /* Lock the load balancing */
    spin_lock(&load_balance_lock);
    
    /* Set the interval */
    load_balance_interval = interval;
    
    /* Unlock the load balancing */
    spin_unlock(&load_balance_lock);
    
    printk(KERN_INFO "LOAD_BALANCE: Set interval to %llu ms\n", interval);
    
    return 0;
}

/**
 * Set the load balancing threshold
 * 
 * @param threshold Threshold in percent
 * @return 0 on success, negative error code on failure
 */
int load_balance_set_threshold(u64 threshold) {
    /* Check parameters */
    if (threshold > 100) {
        return -EINVAL;
    }
    
    /* Lock the load balancing */
    spin_lock(&load_balance_lock);
    
    /* Set the threshold */
    load_balance_threshold = threshold;
    
    /* Unlock the load balancing */
    spin_unlock(&load_balance_lock);
    
    printk(KERN_INFO "LOAD_BALANCE: Set threshold to %llu%%\n", threshold);
    
    return 0;
}

/**
 * Check if load balancing is needed
 * 
 * @return 1 if needed, 0 if not
 */
int load_balance_needed(void) {
    /* Check if load balancing is enabled */
    if (!load_balance_enabled) {
        return 0;
    }
    
    /* Get the current time */
    u64 current_time = timer_get_ticks();
    
    /* Check if it's time to balance */
    if (current_time - load_balance_last_time < load_balance_interval) {
        return 0;
    }
    
    /* Update the last time */
    load_balance_last_time = current_time;
    
    /* Check if there is an imbalance */
    return load_balance_check_imbalance();
}

/**
 * Check if there is an imbalance between CPUs
 * 
 * @return 1 if there is an imbalance, 0 if not
 */
int load_balance_check_imbalance(void) {
    /* Get the number of CPUs */
    int nr_cpus = CONFIG_NR_CPUS;
    
    /* Check if there is more than one CPU */
    if (nr_cpus <= 1) {
        return 0;
    }
    
    /* Find the busiest and idlest CPUs */
    int busiest_cpu = -1;
    int idlest_cpu = -1;
    u32 busiest_load = 0;
    u32 idlest_load = UINT32_MAX;
    
    for (int i = 0; i < nr_cpus; i++) {
        /* Get the run queue */
        run_queue_t *rq = &run_queues[i];
        
        /* Get the load */
        u32 load = rq->nr_running;
        
        /* Check if this is the busiest CPU */
        if (load > busiest_load) {
            busiest_load = load;
            busiest_cpu = i;
        }
        
        /* Check if this is the idlest CPU */
        if (load < idlest_load) {
            idlest_load = load;
            idlest_cpu = i;
        }
    }
    
    /* Check if there is an imbalance */
    if (busiest_cpu != -1 && idlest_cpu != -1 && busiest_cpu != idlest_cpu) {
        /* Calculate the imbalance */
        u32 imbalance = busiest_load - idlest_load;
        
        /* Check if the imbalance is above the threshold */
        if (imbalance > 0 && (imbalance * 100) / (busiest_load + 1) >= load_balance_threshold) {
            /* There is an imbalance */
            load_balance_imbalance++;
            return 1;
        }
    }
    
    return 0;
}

/**
 * Balance the load between CPUs
 * 
 * @return Number of threads moved, or negative error code on failure
 */
int load_balance_run(void) {
    /* Check if load balancing is needed */
    if (!load_balance_needed()) {
        load_balance_skipped++;
        return 0;
    }
    
    /* Lock the load balancing */
    spin_lock(&load_balance_lock);
    
    /* Increment the count */
    load_balance_count++;
    
    /* Get the number of CPUs */
    int nr_cpus = CONFIG_NR_CPUS;
    
    /* Find the busiest and idlest CPUs */
    int busiest_cpu = -1;
    int idlest_cpu = -1;
    u32 busiest_load = 0;
    u32 idlest_load = UINT32_MAX;
    
    for (int i = 0; i < nr_cpus; i++) {
        /* Get the run queue */
        run_queue_t *rq = &run_queues[i];
        
        /* Get the load */
        u32 load = rq->nr_running;
        
        /* Check if this is the busiest CPU */
        if (load > busiest_load) {
            busiest_load = load;
            busiest_cpu = i;
        }
        
        /* Check if this is the idlest CPU */
        if (load < idlest_load) {
            idlest_load = load;
            idlest_cpu = i;
        }
    }
    
    /* Check if there is an imbalance */
    if (busiest_cpu == -1 || idlest_cpu == -1 || busiest_cpu == idlest_cpu) {
        /* No imbalance */
        spin_unlock(&load_balance_lock);
        load_balance_skipped++;
        return 0;
    }
    
    /* Calculate the imbalance */
    u32 imbalance = busiest_load - idlest_load;
    
    /* Check if the imbalance is above the threshold */
    if (imbalance == 0 || (imbalance * 100) / (busiest_load + 1) < load_balance_threshold) {
        /* No significant imbalance */
        spin_unlock(&load_balance_lock);
        load_balance_skipped++;
        return 0;
    }
    
    /* Calculate the number of threads to move */
    u32 nr_to_move = imbalance / 2;
    
    /* Make sure we move at least one thread */
    if (nr_to_move == 0) {
        nr_to_move = 1;
    }
    
    /* Get the run queues */
    run_queue_t *busiest_rq = &run_queues[busiest_cpu];
    run_queue_t *idlest_rq = &run_queues[idlest_cpu];
    
    /* Lock the run queues */
    spin_lock(&busiest_rq->lock);
    spin_lock(&idlest_rq->lock);
    
    /* Move threads from the busiest to the idlest CPU */
    u32 nr_moved = 0;
    
    /* Find threads to move */
    for (int i = 0; i <= SCHED_PRIO_MAX && nr_moved < nr_to_move; i++) {
        /* Check if there are threads at this priority */
        if (list_empty(&busiest_rq->active[i])) {
            continue;
        }
        
        /* Get the first thread at this priority */
        thread_t *thread = list_first_entry(&busiest_rq->active[i], thread_t, sched_list);
        
        /* Check if the thread can be moved */
        if (thread->cpu != busiest_cpu || thread->cpu == idlest_cpu) {
            continue;
        }
        
        /* Remove the thread from the busiest CPU */
        list_del(&thread->sched_list);
        
        /* Update the bitmap */
        if (list_empty(&busiest_rq->active[i])) {
            busiest_rq->bitmap &= ~(1ULL << i);
        }
        
        /* Set the thread's CPU */
        thread->cpu = idlest_cpu;
        thread->rq = idlest_rq;
        
        /* Add the thread to the idlest CPU */
        list_add_tail(&thread->sched_list, &idlest_rq->active[i]);
        
        /* Update the bitmap */
        idlest_rq->bitmap |= (1ULL << i);
        
        /* Update the counts */
        busiest_rq->nr_running--;
        idlest_rq->nr_running++;
        
        /* Increment the moved count */
        nr_moved++;
    }
    
    /* Unlock the run queues */
    spin_unlock(&idlest_rq->lock);
    spin_unlock(&busiest_rq->lock);
    
    /* Update the statistics */
    if (nr_moved > 0) {
        load_balance_moves += nr_moved;
    } else {
        load_balance_failed++;
    }
    
    /* Unlock the load balancing */
    spin_unlock(&load_balance_lock);
    
    return nr_moved;
}

/**
 * Print load balancing statistics
 */
void load_balance_print_stats(void) {
    /* Lock the load balancing */
    spin_lock(&load_balance_lock);
    
    /* Print the statistics */
    printk(KERN_INFO "LOAD_BALANCE: Enabled: %s\n", load_balance_enabled ? "Yes" : "No");
    printk(KERN_INFO "LOAD_BALANCE: Interval: %llu ms\n", load_balance_interval);
    printk(KERN_INFO "LOAD_BALANCE: Threshold: %llu%%\n", load_balance_threshold);
    printk(KERN_INFO "LOAD_BALANCE: Count: %llu\n", load_balance_count);
    printk(KERN_INFO "LOAD_BALANCE: Moves: %llu\n", load_balance_moves);
    printk(KERN_INFO "LOAD_BALANCE: Failed: %llu\n", load_balance_failed);
    printk(KERN_INFO "LOAD_BALANCE: Skipped: %llu\n", load_balance_skipped);
    printk(KERN_INFO "LOAD_BALANCE: Imbalance: %llu\n", load_balance_imbalance);
    
    /* Unlock the load balancing */
    spin_unlock(&load_balance_lock);
}
