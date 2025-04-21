/**
 * sched.h - Advanced scheduler definitions
 *
 * This file contains definitions for the advanced process scheduler.
 */

#ifndef _HORIZON_SCHED_H
#define _HORIZON_SCHED_H

#include <horizon/types.h>
#include <horizon/sched.h>

/* Scheduler policies */
#define SCHED_NORMAL    0   /* Standard round-robin time-sharing policy */
#define SCHED_FIFO      1   /* First in-first out policy */
#define SCHED_RR        2   /* Round-robin policy */
#define SCHED_BATCH     3   /* Batch policy */
#define SCHED_IDLE      4   /* Idle policy */

/* Scheduler parameters */
typedef struct sched_param {
    int sched_priority;     /* Process priority */
} sched_param_t;

/* Process states */
#define TASK_RUNNING        0x0000  /* Process is running or on a run queue */
#define TASK_INTERRUPTIBLE  0x0001  /* Process is sleeping and can be woken up by signals */
#define TASK_UNINTERRUPTIBLE 0x0002 /* Process is sleeping and cannot be woken up by signals */
#define TASK_STOPPED        0x0004  /* Process has been stopped by a signal */
#define TASK_TRACED         0x0008  /* Process is being traced */
#define TASK_ZOMBIE         0x0010  /* Process is a zombie */
#define TASK_DEAD           0x0020  /* Process is dead */

/* Process flags */
#define PF_KTHREAD          0x00000001  /* Kernel thread */
#define PF_STARTING         0x00000002  /* Process is being created */
#define PF_EXITING          0x00000004  /* Process is exiting */
#define PF_FORKNOEXEC       0x00000008  /* Process has been forked but hasn't execed */
#define PF_SUPERPRIV        0x00000010  /* Process has superuser privileges */
#define PF_DUMPCORE         0x00000020  /* Process is dumping core */
#define PF_SIGNALED         0x00000040  /* Process was killed by a signal */
#define PF_MEMALLOC         0x00000080  /* Process is allocating memory */

/* Process credentials */
typedef struct cred {
    u32 uid;                /* User ID */
    u32 gid;                /* Group ID */
    u32 euid;               /* Effective user ID */
    u32 egid;               /* Effective group ID */
    u32 suid;               /* Saved user ID */
    u32 sgid;               /* Saved group ID */
    u32 fsuid;              /* File system user ID */
    u32 fsgid;              /* File system group ID */
    u32 cap_inheritable;    /* Inheritable capabilities */
    u32 cap_permitted;      /* Permitted capabilities */
    u32 cap_effective;      /* Effective capabilities */
} cred_t;

/* Process memory map */
typedef struct mm_struct {
    void *start_code;       /* Start of code segment */
    void *end_code;         /* End of code segment */
    void *start_data;       /* Start of data segment */
    void *end_data;         /* End of data segment */
    void *start_brk;        /* Start of heap */
    void *brk;              /* Current heap pointer */
    void *start_stack;      /* Start of stack */
    void *arg_start;        /* Start of command line arguments */
    void *arg_end;          /* End of command line arguments */
    void *env_start;        /* Start of environment variables */
    void *env_end;          /* End of environment variables */
    struct vm_area_struct *mmap; /* List of memory areas */
} mm_struct_t;

/* Process context */
typedef struct context {
    u32 eip;                /* Instruction pointer */
    u32 esp;                /* Stack pointer */
    u32 ebp;                /* Base pointer */
    u32 ebx;                /* General purpose registers */
    u32 esi;
    u32 edi;
    u32 eflags;             /* Flags */
} context_t;

/* Enhanced process structure */
typedef struct task_struct {
    volatile long state;    /* Process state */
    u32 flags;              /* Process flags */
    u32 pid;                /* Process ID */
    u32 tgid;               /* Thread group ID */
    u32 ppid;               /* Parent process ID */
    char comm[16];          /* Process name */

    /* Scheduler information */
    int prio;               /* Process priority */
    int static_prio;        /* Static priority */
    int normal_prio;        /* Normal priority */
    u32 policy;             /* Scheduling policy */
    u32 time_slice;         /* Time slice */
    u32 exec_start;         /* Start time */
    u32 sum_exec_runtime;   /* Total execution time */
    u64 wake_time;          /* Wake up time in jiffies */

    /* Memory management */
    mm_struct_t *mm;        /* Memory descriptor */

    /* Credentials */
    cred_t *cred;           /* Process credentials */

    /* File system information */
    void *fs;               /* File system information */
    void *files;            /* Open files */

    /* Signal handling */
    void *sighand;          /* Signal handlers */
    void *signal;           /* Signal information */

    /* Context */
    context_t context;      /* CPU context */

    /* Linked list pointers */
    struct task_struct *parent;     /* Parent process */
    struct task_struct *children;   /* Child processes */
    struct task_struct *sibling;    /* Sibling processes */
    struct task_struct *next;       /* Next process in run queue */
    struct task_struct *prev;       /* Previous process in run queue */
} task_struct_t;

/* Run queue structure */
typedef struct run_queue {
    u32 nr_running;         /* Number of runnable processes */
    u32 nr_switches;        /* Number of context switches */
    u32 curr_timestamp;     /* Current timestamp */
    task_struct_t *curr;    /* Currently running process */
    task_struct_t *idle;    /* Idle process */
    task_struct_t *head;    /* Head of the run queue */
    task_struct_t *tail;    /* Tail of the run queue */
} run_queue_t;

/* Advanced scheduler functions */
void sched_init_advanced(void);
task_struct_t *sched_create_task(const char *name, void (*entry)(void), u32 flags);
void sched_destroy_task(task_struct_t *task);
void sched_add_task(task_struct_t *task);
void sched_remove_task(task_struct_t *task);
void sched_set_priority(task_struct_t *task, int priority);
void sched_set_policy(task_struct_t *task, u32 policy);
void sched_yield_advanced(void);
void sched_schedule(void);
task_struct_t *sched_current_task(void);
void sched_sleep_advanced(u32 ms);
void sched_wake_up(task_struct_t *task);
void sched_exit_advanced(int status);

#endif /* _HORIZON_SCHED_H */
