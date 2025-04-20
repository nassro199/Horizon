/**
 * task.h - Horizon kernel task definitions
 * 
 * This file contains definitions for the task subsystem.
 * The definitions are compatible with Linux.
 */

#ifndef _KERNEL_TASK_H
#define _KERNEL_TASK_H

#include <horizon/types.h>
#include <horizon/list.h>
#include <horizon/mm/vmm.h>
#include <horizon/time.h>
#include <horizon/sched.h>

/* Process states (Linux compatible) */
#define TASK_RUNNING        0
#define TASK_INTERRUPTIBLE  1
#define TASK_UNINTERRUPTIBLE 2
#define TASK_ZOMBIE         3
#define TASK_STOPPED        4
#define TASK_TRACED         5
#define TASK_DEAD           6

/* Process flags */
#define PF_KTHREAD          0x00000001 /* Kernel thread */
#define PF_STARTING         0x00000002 /* Process being created */
#define PF_EXITING          0x00000004 /* Process is exiting */
#define PF_EXITPIDONE       0x00000008 /* Exit signal has been sent */
#define PF_FORKNOEXEC       0x00000010 /* Forked but didn't exec */
#define PF_SUPERPRIV        0x00000020 /* Used super-user privileges */
#define PF_DUMPCORE         0x00000040 /* Dumped core */
#define PF_SIGNALED         0x00000080 /* Killed by a signal */
#define PF_MEMALLOC         0x00000100 /* Allocating memory */
#define PF_USED_MATH        0x00000200 /* Used FPU this quantum */
#define PF_NOFREEZE         0x00000400 /* Process should not be frozen */
#define PF_FROZEN           0x00000800 /* Frozen for system suspend */

/* Forward declarations */
struct task_struct;
struct files_struct;
struct fs_struct;
struct signal_struct;
struct sighand_struct;
struct sigpending;
struct thread_struct;

/* Thread information */
typedef struct thread_info {
    struct task_struct *task;      /* Task structure */
    void *exec_domain;             /* Execution domain */
    unsigned long flags;           /* Low level flags */
    unsigned long status;          /* Thread synchronous flags */
    unsigned long cpu;             /* Current CPU */
    int preempt_count;             /* 0 => preemptable, <0 => BUG */
    unsigned long addr_limit;      /* Address limit */
    void *sysenter_return;         /* SYSENTER_RETURN */
} thread_info_t;

/* File structure */
typedef struct file {
    unsigned int f_flags;          /* File flags */
    mode_t f_mode;                 /* File mode */
    loff_t f_pos;                  /* File position */
    unsigned int f_count;          /* Reference count */
    struct file_operations *f_op;  /* File operations */
    void *private_data;            /* Private data */
} file_t;

/* File descriptor structure */
typedef struct files_struct {
    unsigned int count;            /* Reference count */
    unsigned int max_fds;          /* Maximum number of file descriptors */
    unsigned int next_fd;          /* Next free file descriptor */
    file_t **fd_array;             /* Array of file descriptors */
    unsigned int *close_on_exec;   /* Close on exec flags */
} files_struct_t;

/* Filesystem structure */
typedef struct fs_struct {
    unsigned int count;            /* Reference count */
    struct dentry *root;           /* Root directory */
    struct dentry *pwd;            /* Current directory */
    struct vfsmount *rootmnt;      /* Root mount */
    struct vfsmount *pwdmnt;       /* Current mount */
} fs_struct_t;

/* Signal handler structure */
typedef struct sighand_struct {
    unsigned int count;            /* Reference count */
    struct k_sigaction action[64]; /* Signal actions */
} sighand_struct_t;

/* Signal structure */
typedef struct signal_struct {
    unsigned int count;            /* Reference count */
    struct list_head thread_head;  /* Thread list */
    struct task_struct *curr_target; /* Current target */
    struct sigpending shared_pending; /* Shared pending signals */
    int group_exit;                /* Group exit */
    int group_exit_code;           /* Group exit code */
    int group_stop_count;          /* Group stop count */
    unsigned int flags;            /* Signal flags */
    struct list_head posix_timers; /* POSIX timers */
    struct hrtimer real_timer;     /* Real timer */
    struct pid *pids[PIDTYPE_MAX]; /* PIDs */
    struct task_struct *tty_old_pgrp; /* Old TTY process group */
    int leader;                    /* Process group leader */
    struct tty_struct *tty;        /* Controlling TTY */
} signal_struct_t;

/* Thread structure */
typedef struct thread_struct {
    unsigned long sp;              /* Stack pointer */
    unsigned long ip;              /* Instruction pointer */
    unsigned long fs;              /* FS register */
    unsigned long gs;              /* GS register */
    unsigned long es;              /* ES register */
    unsigned long ds;              /* DS register */
    unsigned long ss;              /* SS register */
    unsigned long cr2;             /* CR2 register */
    unsigned long trap_nr;         /* Trap number */
    unsigned long error_code;      /* Error code */
    struct thread_info *thread_info; /* Thread information */
} thread_struct_t;

/* Task structure */
typedef struct task_struct {
    /* Process identification */
    pid_t pid;                     /* Process ID */
    pid_t tgid;                    /* Thread group ID */
    pid_t ppid;                    /* Parent process ID */
    char comm[16];                 /* Command name */
    
    /* Process state */
    volatile long state;           /* Process state */
    unsigned int flags;            /* Process flags */
    int exit_state;                /* Exit state */
    int exit_code;                 /* Exit code */
    int exit_signal;               /* Exit signal */
    
    /* Process hierarchy */
    struct task_struct *parent;    /* Parent process */
    struct task_struct *real_parent; /* Real parent process */
    struct list_head children;     /* Child processes */
    struct list_head sibling;      /* Sibling processes */
    struct task_struct *group_leader; /* Thread group leader */
    
    /* Process scheduling */
    int prio;                      /* Process priority */
    int static_prio;               /* Static priority */
    int normal_prio;               /* Normal priority */
    unsigned int rt_priority;      /* Real-time priority */
    unsigned int policy;           /* Scheduling policy */
    unsigned int time_slice;       /* Time slice */
    int on_rq;                     /* On run queue */
    
    /* Process memory */
    struct mm_struct *mm;          /* Memory descriptor */
    struct mm_struct *active_mm;   /* Active memory descriptor */
    
    /* Process files */
    struct files_struct *files;    /* File descriptors */
    struct fs_struct *fs;          /* Filesystem information */
    
    /* Process signals */
    struct signal_struct *signal;  /* Signal handlers */
    struct sighand_struct *sighand; /* Signal handlers */
    sigset_t blocked;              /* Blocked signals */
    sigset_t real_blocked;         /* Real blocked signals */
    sigset_t saved_sigmask;        /* Saved signal mask */
    struct sigpending pending;     /* Pending signals */
    
    /* Process credentials */
    uid_t uid;                     /* User ID */
    uid_t euid;                    /* Effective user ID */
    uid_t suid;                    /* Saved user ID */
    uid_t fsuid;                   /* Filesystem user ID */
    gid_t gid;                     /* Group ID */
    gid_t egid;                    /* Effective group ID */
    gid_t sgid;                    /* Saved group ID */
    gid_t fsgid;                   /* Filesystem group ID */
    
    /* Process execution */
    struct thread_struct thread;   /* Thread information */
    void *stack;                   /* Kernel stack */
    
    /* Process lists */
    struct list_head tasks;        /* Task list */
    struct list_head thread_group; /* Thread group list */
    
    /* Process times */
    struct timespec start_time;    /* Start time */
    u64 utime;                     /* User time */
    u64 stime;                     /* System time */
    
    /* Process statistics */
    u64 min_flt;                   /* Minor page faults */
    u64 maj_flt;                   /* Major page faults */
    
    /* Process CPU */
    int cpu;                       /* CPU */
    int on_cpu;                    /* On CPU */
} task_struct_t;

/* Task functions */
void task_init(void);
task_struct_t *task_create(const char *name);
int task_destroy(task_struct_t *task);
int task_exec(task_struct_t *task, const char *path, char *const argv[], char *const envp[]);
task_struct_t *task_fork(task_struct_t *parent);
int task_wait(task_struct_t *task, int *status);
int task_exit(task_struct_t *task, int status);
task_struct_t *task_current(void);
task_struct_t *task_get(pid_t pid);
int task_set_state(task_struct_t *task, long state);
int task_set_name(task_struct_t *task, const char *name);
int task_add_file(task_struct_t *task, file_t *file);
int task_remove_file(task_struct_t *task, unsigned int fd);
file_t *task_get_file(task_struct_t *task, unsigned int fd);
int task_signal(task_struct_t *task, int sig);
int task_signal_group(task_struct_t *task, int sig);
int task_signal_all(int sig);

/* Current task */
extern task_struct_t *current;

/* Idle task */
extern task_struct_t *idle_task(int cpu);

/* Init task */
extern task_struct_t init_task;

#endif /* _KERNEL_TASK_H */
