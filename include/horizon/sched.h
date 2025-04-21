/**
 * sched.h - Horizon kernel scheduler definitions
 *
 * This file contains definitions for the kernel scheduler.
 */

#ifndef _HORIZON_SCHED_H
#define _HORIZON_SCHED_H

#include <horizon/types.h>
#include <horizon/list.h>
#include <horizon/config.h>

/* Include scheduler components */
#include <horizon/sched/rt.h>
#include <horizon/sched/sched_domain.h>
#include <horizon/sched/load_balance.h>

/* Forward declarations */
struct thread;

/* Scheduler policies */
#define SCHED_NORMAL    0  /* Normal scheduling policy */
#define SCHED_FIFO      1  /* First-in, first-out scheduling policy */
#define SCHED_RR        2  /* Round-robin scheduling policy */
#define SCHED_BATCH     3  /* Batch scheduling policy */
#define SCHED_IDLE      4  /* Idle scheduling policy */
#define SCHED_DEADLINE  5  /* Deadline scheduling policy */

/* Scheduler priorities */
#define SCHED_PRIO_MIN   0   /* Minimum priority */
#define SCHED_PRIO_DEFAULT 20 /* Default priority */
#define SCHED_PRIO_MAX   99  /* Maximum priority */

/* Scheduler time slice */
#define SCHED_TIMESLICE_DEFAULT 100  /* Default time slice in milliseconds */

/* Scheduler run queue */
typedef struct run_queue {
    /* Run queue lock */
    u32 lock;

    /* Run queue statistics */
    u32 nr_running;           /* Number of running threads */
    u32 nr_switches;          /* Number of context switches */
    u64 nr_schedule;          /* Number of schedules */
    u64 curr_timestamp;       /* Current timestamp */
    u64 last_timestamp;       /* Last timestamp */

    /* Run queue lists */
    struct list_head queue;   /* Run queue */
    struct list_head expired; /* Expired queue */

    /* Run queue arrays */
    struct list_head (*active)[SCHED_PRIO_MAX + 1];  /* Active array */
    struct list_head arrays[2][SCHED_PRIO_MAX + 1]; /* Priority arrays */

    /* Run queue current */
    struct thread *curr;      /* Current thread */
    struct thread *idle;      /* Idle thread */
    struct thread *head;      /* Head of the run queue */
    struct thread *tail;      /* Tail of the run queue */

    /* Run queue bitmap */
    u64 bitmap;               /* Priority bitmap */
} run_queue_t;

/* Scheduler functions */
void sched_init(void);
void sched_start(void);
void sched_stop(void);
void sched_tick(void);
void sched_yield(void);
void sched_schedule(void);
void sched_add_thread(struct thread *thread);
void sched_remove_thread(struct thread *thread);
void sched_block_thread(struct thread *thread);
void sched_unblock_thread(struct thread *thread);
void sched_sleep_thread(struct thread *thread, u64 ms);
void sched_wakeup_thread(struct thread *thread);
void sched_set_priority(struct thread *thread, int priority);
int sched_get_priority(struct thread *thread);
void sched_set_policy(struct thread *thread, u32 policy);
u32 sched_get_policy(struct thread *thread);
void sched_set_timeslice(struct thread *thread, u64 timeslice);
u64 sched_get_timeslice(struct thread *thread);
void sched_set_affinity(struct thread *thread, u32 cpu);
u32 sched_get_affinity(struct thread *thread);
void sched_switch(struct thread *prev, struct thread *next);
void sched_enqueue_thread(struct thread *thread);
struct thread *sched_dequeue_thread(void);
void sched_requeue_thread(struct thread *thread);
void sched_check_preempt(struct thread *thread);
void sched_check_expired(run_queue_t *rq);
void sched_update_thread(struct thread *thread);
void sched_update_priority(struct thread *thread);
void sched_update_timeslice(struct thread *thread);
void sched_update_policy(struct thread *thread);
void sched_update_affinity(struct thread *thread);
void sched_update_statistics(run_queue_t *rq);
void sched_print_statistics(void);
void sched_print_runqueue(void);
void sched_print_thread(struct thread *thread);

/* Scheduler context switch */
void sched_context_switch(struct thread *prev, struct thread *next);

/* Scheduler idle thread */
void sched_idle_thread(void);

/* Scheduler run queues */
extern run_queue_t run_queues[CONFIG_NR_CPUS];

/* Current run queue */
#define this_rq() (&run_queues[0])

/* Current thread */
#define current_thread (this_rq()->curr)

#endif /* _HORIZON_SCHED_H */
