/**
 * sched.h - Scheduler definitions
 * 
 * This file contains definitions for the process scheduler.
 */

#ifndef _SCHED_H
#define _SCHED_H

#include <horizon/types.h>

/* Process states */
typedef enum {
    PROCESS_RUNNING,
    PROCESS_SLEEPING,
    PROCESS_ZOMBIE,
    PROCESS_STOPPED
} process_state_t;

/* Process priority */
#define PROCESS_PRIO_MIN  0
#define PROCESS_PRIO_MAX  99
#define PROCESS_PRIO_DEFAULT 20

/* Process structure */
typedef struct process {
    u32 pid;                  /* Process ID */
    u32 ppid;                 /* Parent process ID */
    char name[64];            /* Process name */
    process_state_t state;    /* Process state */
    u32 priority;             /* Process priority */
    void *stack;              /* Process stack */
    void *mm;                 /* Memory map */
    struct process *next;     /* Next process in list */
} process_t;

/* Scheduler functions */
void sched_init(void);
process_t *sched_create_process(const char *name, void (*entry)(void));
void sched_yield(void);
void sched_sleep(u32 ms);
void sched_exit(int status);

#endif /* _SCHED_H */
