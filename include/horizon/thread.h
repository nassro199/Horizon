/**
 * thread.h - Horizon kernel thread definitions
 *
 * This file contains definitions for kernel threads.
 */

#ifndef _HORIZON_THREAD_H
#define _HORIZON_THREAD_H

#include <horizon/types.h>
#include <horizon/list.h>
#include <horizon/signal.h>

/* Thread flags */
#define THREAD_KERNEL     0x00000001  /* Kernel thread */
#define THREAD_USER       0x00000002  /* User thread */
#define THREAD_JOINABLE   0x00000004  /* Thread can be joined */
#define THREAD_DETACHED   0x00000008  /* Thread is detached */
#define THREAD_EXITING    0x00000010  /* Thread is exiting */
#define THREAD_DEAD       0x00000020  /* Thread is dead */
#define THREAD_STOPPED    0x00000040  /* Thread is stopped */
#define THREAD_INTERRUPTIBLE 0x00000080 /* Thread is interruptible */
#define THREAD_UNINTERRUPTIBLE 0x00000100 /* Thread is uninterruptible */

/* Thread states */
#define THREAD_STATE_RUNNING      0   /* Thread is running */
#define THREAD_STATE_READY        1   /* Thread is ready to run */
#define THREAD_STATE_BLOCKED      2   /* Thread is blocked */
#define THREAD_STATE_SLEEPING     3   /* Thread is sleeping */
#define THREAD_STATE_STOPPED      4   /* Thread is stopped */
#define THREAD_STATE_ZOMBIE       5   /* Thread is zombie */
#define THREAD_STATE_DEAD         6   /* Thread is dead */

/* Thread priorities */
#define THREAD_PRIO_IDLE          0   /* Idle priority */
#define THREAD_PRIO_LOW           1   /* Low priority */
#define THREAD_PRIO_NORMAL        2   /* Normal priority */
#define THREAD_PRIO_HIGH          3   /* High priority */
#define THREAD_PRIO_REALTIME      4   /* Real-time priority */

/* Thread scheduling policies */
#define THREAD_SCHED_OTHER        0   /* Normal scheduling */
#define THREAD_SCHED_FIFO         1   /* First-in, first-out scheduling */
#define THREAD_SCHED_RR           2   /* Round-robin scheduling */
#define THREAD_SCHED_BATCH        3   /* Batch scheduling */
#define THREAD_SCHED_IDLE         4   /* Idle scheduling */
#define THREAD_SCHED_DEADLINE     5   /* Deadline scheduling */

/* Thread structure */
typedef struct thread {
    /* Thread identification */
    tid_t tid;                      /* Thread ID */
    pid_t pid;                      /* Process ID */

    /* Thread state */
    u32 state;                      /* Thread state */
    u32 flags;                      /* Thread flags */
    int exit_code;                  /* Exit code */

    /* Thread scheduling */
    u32 policy;                     /* Scheduling policy */
    int priority;                   /* Thread priority */
    int static_priority;            /* Static priority */
    int dynamic_priority;           /* Dynamic priority */
    u64 time_slice;                 /* Time slice */
    u64 start_time;                 /* Start time */
    u64 user_time;                  /* User time */
    u64 system_time;                /* System time */
    u32 cpu;                        /* CPU */
    u32 on_cpu;                     /* On CPU */

    /* Thread context */
    void *kernel_stack;             /* Kernel stack */
    void *user_stack;               /* User stack */
    void *tls;                      /* Thread-local storage */
    void *context;                  /* Thread context */

    /* Thread synchronization */
    void *blocked_on;               /* Object thread is blocked on */
    u64 wakeup_time;                /* Wakeup time */

    /* Thread signals */
    sigset_t signal_mask;           /* Signal mask */
    sigset_t saved_signal_mask;     /* Saved signal mask */
    struct sigaction *sigactions;   /* Signal actions */

    /* Thread lists */
    list_head_t thread_list;        /* Thread list */
    list_head_t process_threads;    /* Process threads */

    /* Thread function */
    void *(*start_routine)(void *); /* Thread function */
    void *arg;                      /* Thread argument */
    void *retval;                   /* Thread return value */

    /* Thread cleanup */
    void (*cleanup_handler)(void *); /* Cleanup handler */
    void *cleanup_arg;              /* Cleanup argument */

    /* Thread-specific data */
    void **tsd;                     /* Thread-specific data */
    u32 tsd_count;                  /* Thread-specific data count */

    /* Thread owner */
    struct task_struct *task;       /* Owner task */
} thread_t;

/* Thread functions */
void thread_init(void);
thread_t *thread_create(void *(*start_routine)(void *), void *arg, u32 flags);
int thread_start(thread_t *thread);
int thread_join(thread_t *thread, void **retval);
int thread_detach(thread_t *thread);
int thread_cancel(thread_t *thread);
int thread_exit(void *retval);
thread_t *thread_self(void);
int thread_yield(void);
int thread_sleep(u64 ms);
int thread_wakeup(thread_t *thread);
int thread_set_priority(thread_t *thread, int priority);
int thread_get_priority(thread_t *thread);
int thread_set_policy(thread_t *thread, u32 policy);
u32 thread_get_policy(thread_t *thread);
int thread_set_affinity(thread_t *thread, u32 cpu);
u32 thread_get_affinity(thread_t *thread);
int thread_set_name(thread_t *thread, const char *name);
int thread_get_name(thread_t *thread, char *name, size_t size);
int thread_set_tls(thread_t *thread, void *tls);
void *thread_get_tls(thread_t *thread);
int thread_set_tsd(thread_t *thread, u32 key, void *value);
void *thread_get_tsd(thread_t *thread, u32 key);
int thread_key_create(u32 *key, void (*destructor)(void *));
int thread_key_delete(u32 key);

#endif /* _HORIZON_THREAD_H */
