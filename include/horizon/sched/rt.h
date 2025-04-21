/**
 * rt.h - Horizon kernel real-time scheduler definitions
 * 
 * This file contains definitions for real-time scheduling policies.
 */

#ifndef _HORIZON_SCHED_RT_H
#define _HORIZON_SCHED_RT_H

#include <horizon/types.h>

/* Real-time scheduling policies */
#define SCHED_NORMAL 0 /* Normal (non-real-time) scheduling */
#define SCHED_FIFO   1 /* First-in, first-out real-time scheduling */
#define SCHED_RR     2 /* Round-robin real-time scheduling */
#define SCHED_BATCH  3 /* Batch scheduling */
#define SCHED_IDLE   4 /* Idle scheduling */
#define SCHED_DEADLINE 5 /* Deadline scheduling */

/* Forward declarations */
struct thread;
struct run_queue;

/* Initialize the real-time scheduler */
void rt_init(void);

/* Enable or disable real-time scheduling */
int rt_enable(int enable);

/* Set the real-time runtime */
int rt_set_runtime(u64 runtime);

/* Set the real-time period */
int rt_set_period(u64 period);

/* Set the real-time priority base */
int rt_set_prio_base(u32 prio_base);

/* Check if a thread is real-time */
int rt_is_realtime(struct thread *thread);

/* Check if a thread can preempt another thread */
int rt_can_preempt(struct thread *thread, struct thread *current);

/* Schedule a real-time thread */
struct thread *rt_schedule(struct run_queue *rq);

/* Enqueue a real-time thread */
int rt_enqueue(struct run_queue *rq, struct thread *thread);

/* Dequeue a real-time thread */
int rt_dequeue(struct run_queue *rq, struct thread *thread);

/* Yield a real-time thread */
int rt_yield(struct run_queue *rq, struct thread *thread);

/* Boost a real-time thread's priority */
int rt_boost(struct thread *thread, int boost);

/* Throttle a real-time thread */
int rt_throttle(struct thread *thread, int throttle);

/* Print real-time scheduler statistics */
void rt_print_stats(void);

#endif /* _HORIZON_SCHED_RT_H */
