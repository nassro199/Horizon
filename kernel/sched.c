/**
 * sched.c - Horizon kernel scheduler implementation
 *
 * This file contains the implementation of the kernel scheduler.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/sched.h>
#include <horizon/thread.h>
#include <horizon/task.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <horizon/time.h>
#include <horizon/console.h>

/* Scheduler run queues */
run_queue_t run_queues[CONFIG_NR_CPUS];

/* Scheduler initialization */
void sched_init(void) {
    /* Initialize run queues */
    for (u32 i = 0; i < CONFIG_NR_CPUS; i++) {
        run_queue_t *rq = &run_queues[i];

        /* Initialize run queue */
        memset(rq, 0, sizeof(run_queue_t));

        /* Initialize run queue lists */
        INIT_LIST_HEAD(&rq->queue);
        INIT_LIST_HEAD(&rq->expired);

        /* Initialize run queue arrays */
        for (u32 j = 0; j <= SCHED_PRIO_MAX; j++) {
            INIT_LIST_HEAD(&rq->arrays[0][j]);
            INIT_LIST_HEAD(&rq->arrays[1][j]);
        }

        /* Initialize run queue pointers */
        rq->active = rq->arrays[0];
        rq->expired = rq->arrays[1];

        /* Initialize run queue statistics */
        rq->nr_running = 0;
        rq->nr_switches = 0;
        rq->nr_schedule = 0;
        rq->curr_timestamp = get_timestamp();
        rq->last_timestamp = rq->curr_timestamp;
    }

    /* Create idle thread */
    thread_t *idle_thread = kmalloc(sizeof(thread_t));
    if (idle_thread != NULL) {
        /* Initialize idle thread */
        memset(idle_thread, 0, sizeof(thread_t));
        idle_thread->tid = 0;
        idle_thread->pid = 0;
        idle_thread->state = THREAD_STATE_RUNNING;
        idle_thread->flags = THREAD_KERNEL;
        idle_thread->priority = THREAD_PRIO_IDLE;
        idle_thread->static_priority = THREAD_PRIO_IDLE;
        idle_thread->dynamic_priority = THREAD_PRIO_IDLE;
        idle_thread->policy = THREAD_SCHED_IDLE;
        idle_thread->time_slice = SCHED_TIMESLICE_DEFAULT;
        idle_thread->start_time = get_timestamp();
        idle_thread->start_routine = (void *(*)(void *))sched_idle_thread;
        idle_thread->arg = NULL;
        idle_thread->task = task_current();

        /* Initialize idle thread lists */
        INIT_LIST_HEAD(&idle_thread->thread_list);
        INIT_LIST_HEAD(&idle_thread->process_threads);

        /* Set idle thread for each run queue */
        for (u32 i = 0; i < CONFIG_NR_CPUS; i++) {
            run_queues[i].idle = idle_thread;
            run_queues[i].curr = idle_thread;
        }
    }
}

/**
 * Start scheduler
 */
void sched_start(void) {
    /* Enable preemption */
    preempt_enable();

    /* Enable interrupts */
    sti();

    /* Start scheduling */
    sched_schedule();
}

/**
 * Stop scheduler
 */
void sched_stop(void) {
    /* Disable interrupts */
    cli();

    /* Disable preemption */
    preempt_disable();
}

/**
 * Scheduler tick
 *
 * This function is called by the timer interrupt handler.
 */
void sched_tick(void) {
    /* Get current run queue */
    run_queue_t *rq = this_rq();

    /* Update timestamp */
    rq->curr_timestamp = get_timestamp();

    /* Get current thread */
    thread_t *curr = rq->curr;

    /* Check if current thread is idle */
    if (curr == rq->idle) {
        return;
    }

    /* Update thread time slice */
    if (curr->time_slice > 0) {
        curr->time_slice--;
    }

    /* Check if thread time slice expired */
    if (curr->time_slice == 0) {
        /* Reset time slice */
        curr->time_slice = SCHED_TIMESLICE_DEFAULT;

        /* Requeue thread */
        sched_requeue_thread(curr);

        /* Schedule */
        sched_schedule();
    }

    /* Check sleeping threads */
    thread_t *thread;
    list_for_each_entry(thread, &rq->expired, thread_list) {
        /* Check if thread is sleeping */
        if (thread->state == THREAD_STATE_SLEEPING) {
            /* Check if thread wakeup time has passed */
            if (rq->curr_timestamp >= thread->wakeup_time) {
                /* Wake up thread */
                thread->state = THREAD_STATE_READY;

                /* Add thread to run queue */
                sched_add_thread(thread);
            }
        }
    }

    /* Check expired queue */
    sched_check_expired(rq);

    /* Update statistics */
    sched_update_statistics(rq);
}

/**
 * Yield the CPU
 */
void sched_yield(void) {
    /* Schedule */
    sched_schedule();
}

/**
 * Schedule
 *
 * This function selects the next thread to run.
 */
void sched_schedule(void) {
    /* Disable interrupts */
    cli();

    /* Get current run queue */
    run_queue_t *rq = this_rq();

    /* Update statistics */
    rq->nr_schedule++;

    /* Get current thread */
    thread_t *curr = rq->curr;

    /* Get next thread */
    thread_t *next = sched_dequeue_thread();

    /* If no thread is ready, use idle thread */
    if (next == NULL) {
        next = rq->idle;
    }

    /* If current thread is still running, requeue it */
    if (curr != rq->idle && curr->state == THREAD_STATE_RUNNING) {
        /* Requeue thread */
        sched_requeue_thread(curr);
    }

    /* Switch to next thread */
    if (curr != next) {
        /* Update statistics */
        rq->nr_switches++;

        /* Set thread state */
        curr->state = THREAD_STATE_READY;
        next->state = THREAD_STATE_RUNNING;

        /* Set current thread */
        rq->curr = next;

        /* Switch context */
        sched_context_switch(curr, next);
    }

    /* Enable interrupts */
    sti();
}

/**
 * Add a thread to the run queue
 *
 * @param thread Thread to add
 */
void sched_add_thread(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Get run queue */
    run_queue_t *rq = this_rq();

    /* Check thread state */
    if (thread->state != THREAD_STATE_READY) {
        return;
    }

    /* Add thread to run queue */
    sched_enqueue_thread(thread);

    /* Update statistics */
    rq->nr_running++;
}

/**
 * Remove a thread from the run queue
 *
 * @param thread Thread to remove
 */
void sched_remove_thread(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Get run queue */
    run_queue_t *rq = this_rq();

    /* Remove thread from run queue */
    list_del(&thread->thread_list);

    /* Update statistics */
    if (rq->nr_running > 0) {
        rq->nr_running--;
    }
}

/**
 * Block a thread
 *
 * @param thread Thread to block
 */
void sched_block_thread(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Set thread state */
    thread->state = THREAD_STATE_BLOCKED;

    /* Remove thread from run queue */
    sched_remove_thread(thread);

    /* If thread is current thread, schedule */
    if (thread == this_rq()->curr) {
        sched_schedule();
    }
}

/**
 * Unblock a thread
 *
 * @param thread Thread to unblock
 */
void sched_unblock_thread(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Check thread state */
    if (thread->state != THREAD_STATE_BLOCKED) {
        return;
    }

    /* Set thread state */
    thread->state = THREAD_STATE_READY;

    /* Add thread to run queue */
    sched_add_thread(thread);

    /* Check preemption */
    sched_check_preempt(thread);
}

/**
 * Sleep a thread
 *
 * @param thread Thread to sleep
 * @param ms Time to sleep in milliseconds
 */
void sched_sleep_thread(struct thread *thread, u64 ms) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Set thread state */
    thread->state = THREAD_STATE_SLEEPING;

    /* Set wakeup time */
    thread->wakeup_time = get_timestamp() + ms * 1000;

    /* Remove thread from run queue */
    sched_remove_thread(thread);

    /* Add thread to expired queue */
    list_add_tail(&thread->thread_list, &this_rq()->expired);

    /* If thread is current thread, schedule */
    if (thread == this_rq()->curr) {
        sched_schedule();
    }
}

/**
 * Wake up a thread
 *
 * @param thread Thread to wake up
 */
void sched_wakeup_thread(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Check thread state */
    if (thread->state != THREAD_STATE_SLEEPING) {
        return;
    }

    /* Set thread state */
    thread->state = THREAD_STATE_READY;

    /* Remove thread from expired queue */
    list_del(&thread->thread_list);

    /* Add thread to run queue */
    sched_add_thread(thread);

    /* Check preemption */
    sched_check_preempt(thread);
}

/**
 * Set thread priority
 *
 * @param thread Thread to set priority
 * @param priority Priority
 */
void sched_set_priority(struct thread *thread, int priority) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Check priority range */
    if (priority < THREAD_PRIO_IDLE || priority > THREAD_PRIO_REALTIME) {
        return;
    }

    /* Set priority */
    thread->priority = priority;
    thread->static_priority = priority;
    thread->dynamic_priority = priority;

    /* Update thread */
    sched_update_thread(thread);
}

/**
 * Get thread priority
 *
 * @param thread Thread to get priority
 * @return Priority
 */
int sched_get_priority(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }

    /* Get priority */
    return thread->priority;
}

/**
 * Set thread scheduling policy
 *
 * @param thread Thread to set policy
 * @param policy Policy
 */
void sched_set_policy(struct thread *thread, u32 policy) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Check policy range */
    if (policy > THREAD_SCHED_DEADLINE) {
        return;
    }

    /* Set policy */
    thread->policy = policy;

    /* Update thread */
    sched_update_thread(thread);
}

/**
 * Get thread scheduling policy
 *
 * @param thread Thread to get policy
 * @return Policy
 */
u32 sched_get_policy(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }

    /* Get policy */
    return thread->policy;
}

/**
 * Set thread time slice
 *
 * @param thread Thread to set time slice
 * @param timeslice Time slice
 */
void sched_set_timeslice(struct thread *thread, u64 timeslice) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Set time slice */
    thread->time_slice = timeslice;
}

/**
 * Get thread time slice
 *
 * @param thread Thread to get time slice
 * @return Time slice
 */
u64 sched_get_timeslice(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return 0;
    }

    /* Get time slice */
    return thread->time_slice;
}

/**
 * Set thread CPU affinity
 *
 * @param thread Thread to set affinity
 * @param cpu CPU
 */
void sched_set_affinity(struct thread *thread, u32 cpu) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Check CPU range */
    if (cpu >= CONFIG_NR_CPUS) {
        return;
    }

    /* Set CPU */
    thread->cpu = cpu;
}

/**
 * Get thread CPU affinity
 *
 * @param thread Thread to get affinity
 * @return CPU
 */
u32 sched_get_affinity(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return 0;
    }

    /* Get CPU */
    return thread->cpu;
}

/**
 * Switch to a new thread
 *
 * @param prev Previous thread
 * @param next Next thread
 */
void sched_switch(struct thread *prev, struct thread *next) {
    /* Check parameters */
    if (prev == NULL || next == NULL) {
        return;
    }

    /* Switch context */
    sched_context_switch(prev, next);
}

/**
 * Enqueue a thread
 *
 * @param thread Thread to enqueue
 */
void sched_enqueue_thread(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Get run queue */
    run_queue_t *rq = this_rq();

    /* Get thread priority */
    u32 prio = thread->dynamic_priority;

    /* Check priority range */
    if (prio > SCHED_PRIO_MAX) {
        prio = SCHED_PRIO_MAX;
    }

    /* Add thread to active array */
    list_add_tail(&thread->thread_list, &rq->active[prio]);

    /* Set bitmap */
    rq->bitmap |= (1ULL << prio);
}

/**
 * Dequeue a thread
 *
 * @return Thread, or NULL if no thread is ready
 */
struct thread *sched_dequeue_thread(void) {
    /* Get run queue */
    run_queue_t *rq = this_rq();

    /* Check if run queue is empty */
    if (rq->bitmap == 0) {
        return NULL;
    }

    /* Find highest priority */
    u32 prio = __builtin_ffsll(rq->bitmap) - 1;

    /* Get thread */
    thread_t *thread = list_first_entry_or_null(&rq->active[prio], thread_t, thread_list);

    /* Check if thread exists */
    if (thread == NULL) {
        /* Clear bitmap */
        rq->bitmap &= ~(1ULL << prio);

        /* Try again */
        return sched_dequeue_thread();
    }

    /* Remove thread from active array */
    list_del(&thread->thread_list);

    /* Check if active array is empty */
    if (list_empty(&rq->active[prio])) {
        /* Clear bitmap */
        rq->bitmap &= ~(1ULL << prio);
    }

    return thread;
}

/**
 * Requeue a thread
 *
 * @param thread Thread to requeue
 */
void sched_requeue_thread(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Get run queue */
    run_queue_t *rq = this_rq();

    /* Get thread priority */
    u32 prio = thread->dynamic_priority;

    /* Check priority range */
    if (prio > SCHED_PRIO_MAX) {
        prio = SCHED_PRIO_MAX;
    }

    /* Add thread to active array */
    list_add_tail(&thread->thread_list, &rq->active[prio]);

    /* Set bitmap */
    rq->bitmap |= (1ULL << prio);
}

/**
 * Check if a thread should preempt the current thread
 *
 * @param thread Thread to check
 */
void sched_check_preempt(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Get run queue */
    run_queue_t *rq = this_rq();

    /* Get current thread */
    thread_t *curr = rq->curr;

    /* Check if current thread is idle */
    if (curr == rq->idle) {
        /* Schedule */
        sched_schedule();
        return;
    }

    /* Check if thread priority is higher than current thread */
    if (thread->dynamic_priority < curr->dynamic_priority) {
        /* Schedule */
        sched_schedule();
    }
}

/**
 * Check if the active array is empty
 *
 * @param rq Run queue
 */
void sched_check_expired(run_queue_t *rq) {
    /* Check parameters */
    if (rq == NULL) {
        return;
    }

    /* Check if active array is empty */
    if (rq->bitmap == 0) {
        /* Swap active and expired arrays */
        struct list_head *tmp = rq->active;
        rq->active = rq->expired;
        rq->expired = tmp;
    }
}

/**
 * Update thread
 *
 * @param thread Thread to update
 */
void sched_update_thread(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Update priority */
    sched_update_priority(thread);

    /* Update time slice */
    sched_update_timeslice(thread);

    /* Update policy */
    sched_update_policy(thread);

    /* Update affinity */
    sched_update_affinity(thread);
}

/**
 * Update thread priority
 *
 * @param thread Thread to update
 */
void sched_update_priority(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Update dynamic priority */
    thread->dynamic_priority = thread->static_priority;
}

/**
 * Update thread time slice
 *
 * @param thread Thread to update
 */
void sched_update_timeslice(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Update time slice based on policy */
    switch (thread->policy) {
        case THREAD_SCHED_FIFO:
            /* FIFO threads don't have a time slice */
            thread->time_slice = 0;
            break;
        case THREAD_SCHED_RR:
            /* Round-robin threads have a time slice */
            thread->time_slice = SCHED_TIMESLICE_DEFAULT;
            break;
        case THREAD_SCHED_OTHER:
        default:
            /* Normal threads have a time slice */
            thread->time_slice = SCHED_TIMESLICE_DEFAULT;
            break;
    }
}

/**
 * Update thread policy
 *
 * @param thread Thread to update
 */
void sched_update_policy(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Update policy */
    switch (thread->policy) {
        case THREAD_SCHED_FIFO:
            /* FIFO threads have high priority */
            thread->static_priority = THREAD_PRIO_REALTIME;
            break;
        case THREAD_SCHED_RR:
            /* Round-robin threads have high priority */
            thread->static_priority = THREAD_PRIO_HIGH;
            break;
        case THREAD_SCHED_OTHER:
        default:
            /* Normal threads have normal priority */
            thread->static_priority = THREAD_PRIO_NORMAL;
            break;
    }

    /* Update dynamic priority */
    thread->dynamic_priority = thread->static_priority;
}

/**
 * Update thread affinity
 *
 * @param thread Thread to update
 */
void sched_update_affinity(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Check CPU range */
    if (thread->cpu >= CONFIG_NR_CPUS) {
        thread->cpu = 0;
    }
}

/**
 * Update run queue statistics
 *
 * @param rq Run queue
 */
void sched_update_statistics(run_queue_t *rq) {
    /* Check parameters */
    if (rq == NULL) {
        return;
    }

    /* Update timestamp */
    rq->last_timestamp = rq->curr_timestamp;
}

/**
 * Print scheduler statistics
 */
void sched_print_statistics(void) {
    /* Get run queue */
    run_queue_t *rq = this_rq();

    /* Print statistics */
    console_printf("Scheduler statistics:\n");
    console_printf("  Running threads: %u\n", rq->nr_running);
    console_printf("  Context switches: %u\n", rq->nr_switches);
    console_printf("  Schedules: %llu\n", rq->nr_schedule);
}

/**
 * Print run queue
 */
void sched_print_runqueue(void) {
    /* Get run queue */
    run_queue_t *rq = this_rq();

    /* Print run queue */
    console_printf("Run queue:\n");

    /* Print active array */
    console_printf("  Active array:\n");
    for (u32 i = 0; i <= SCHED_PRIO_MAX; i++) {
        if (!list_empty(&rq->active[i])) {
            console_printf("    Priority %u:\n", i);

            /* Print threads */
            thread_t *thread;
            list_for_each_entry(thread, &rq->active[i], thread_list) {
                console_printf("      Thread %u (PID %u)\n", thread->tid, thread->pid);
            }
        }
    }

    /* Print expired array */
    console_printf("  Expired array:\n");
    for (u32 i = 0; i <= SCHED_PRIO_MAX; i++) {
        if (!list_empty(&rq->expired[i])) {
            console_printf("    Priority %u:\n", i);

            /* Print threads */
            thread_t *thread;
            list_for_each_entry(thread, &rq->expired[i], thread_list) {
                console_printf("      Thread %u (PID %u)\n", thread->tid, thread->pid);
            }
        }
    }
}

/**
 * Print thread information
 *
 * @param thread Thread to print
 */
void sched_print_thread(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return;
    }

    /* Print thread information */
    console_printf("Thread %u (PID %u):\n", thread->tid, thread->pid);
    console_printf("  State: %u\n", thread->state);
    console_printf("  Flags: 0x%x\n", thread->flags);
    console_printf("  Priority: %d\n", thread->priority);
    console_printf("  Static priority: %d\n", thread->static_priority);
    console_printf("  Dynamic priority: %d\n", thread->dynamic_priority);
    console_printf("  Policy: %u\n", thread->policy);
    console_printf("  Time slice: %llu\n", thread->time_slice);
    console_printf("  CPU: %u\n", thread->cpu);
}

/**
 * Idle thread
 *
 * This function is called when no thread is ready to run.
 */
void sched_idle_thread(void) {
    /* Idle loop */
    for (;;) {
        /* Halt the CPU */
        __asm__ volatile("hlt");
    }
}

/**
 * Context switch
 *
 * @param prev Previous thread
 * @param next Next thread
 */
void sched_context_switch(struct thread *prev, struct thread *next) {
    /* Check parameters */
    if (prev == NULL || next == NULL) {
        return;
    }

    /* Save previous thread context */
    thread_context_t *prev_context = (thread_context_t *)prev->context;

    /* Load next thread context */
    thread_context_t *next_context = (thread_context_t *)next->context;

    /* Switch context */
    __asm__ volatile(
        "pushl %%ebp\n"
        "movl %%esp, %0\n"
        "movl %1, %%esp\n"
        "movl $1f, %0\n"
        "pushl %2\n"
        "ret\n"
        "1:\n"
        "popl %%ebp\n"
        : "=m" (prev_context->esp), "=m" (prev_context->eip)
        : "m" (next_context->eip), "m" (next_context->esp)
        : "memory"
    );
}
