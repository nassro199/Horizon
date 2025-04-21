/**
 * config.h - Horizon kernel scheduler configuration
 *
 * This file contains configuration for the scheduler.
 */

#ifndef _HORIZON_SCHED_CONFIG_H
#define _HORIZON_SCHED_CONFIG_H

/* Number of CPUs */
#define CONFIG_NR_CPUS 1

/* Scheduler timeslice */
#define SCHED_TIMESLICE_DEFAULT 100

/* Scheduler priority levels */
#define SCHED_PRIO_MAX 139
#define SCHED_PRIO_MIN 0

/* Thread priority levels */
#define THREAD_PRIO_IDLE 139
#define THREAD_PRIO_LOW 120
#define THREAD_PRIO_NORMAL 100
#define THREAD_PRIO_HIGH 80
#define THREAD_PRIO_REALTIME 0

/* Thread states */
#define THREAD_STATE_CREATED 0
#define THREAD_STATE_READY 1
#define THREAD_STATE_RUNNING 2
#define THREAD_STATE_BLOCKED 3
#define THREAD_STATE_SLEEPING 4
#define THREAD_STATE_ZOMBIE 5

/* Thread scheduling policies */
#define THREAD_SCHED_OTHER 0
#define THREAD_SCHED_FIFO 1
#define THREAD_SCHED_RR 2
#define THREAD_SCHED_BATCH 3
#define THREAD_SCHED_IDLE 4
#define THREAD_SCHED_DEADLINE 5

#endif /* _HORIZON_SCHED_CONFIG_H */
