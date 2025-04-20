/**
 * process.h - Horizon kernel process definitions
 *
 * This file contains definitions for the process subsystem.
 * The definitions are compatible with Linux.
 */

#ifndef _KERNEL_PROCESS_H
#define _KERNEL_PROCESS_H

#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/vmm.h>
#include <horizon/fs.h>
#include <horizon/security.h>
#include <horizon/list.h>

/* Process states */
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
#define PF_FSTRANS          0x00001000 /* Inside a filesystem transaction */
#define PF_KSWAPD           0x00002000 /* Kswapd process */
#define PF_MEMALLOC_NOIO    0x00004000 /* Allocating memory without IO */
#define PF_LESS_THROTTLE    0x00008000 /* Throttle less: prefer to swap */
#define PF_KTHREAD_BOUND    0x00010000 /* Bound to a CPU */
#define PF_RANDOMIZE        0x00020000 /* Randomize virtual address space */
#define PF_SWAPWRITE        0x00040000 /* Allowed to write to swap */
#define PF_NO_SETAFFINITY   0x00080000 /* Don't set CPU affinity */
#define PF_MCE_EARLY        0x00100000 /* Early kill for mce process policy */
#define PF_MUTEX_TESTER     0x00200000 /* Thread belongs to the rt mutex tester */
#define PF_FREEZER_SKIP     0x00400000 /* Freezer should not count it as freezable */
#define PF_SUSPEND_TASK     0x00800000 /* This thread called freeze_processes */

/* Process memory structure */
typedef struct mm_struct {
    vm_context_t *context;          /* Virtual memory context */
    void *start_code;               /* Start of code segment */
    void *end_code;                 /* End of code segment */
    void *start_data;               /* Start of data segment */
    void *end_data;                 /* End of data segment */
    void *start_brk;                /* Start of heap */
    void *brk;                      /* Current heap end */
    void *start_stack;              /* Start of stack */
    void *arg_start;                /* Start of arguments */
    void *arg_end;                  /* End of arguments */
    void *env_start;                /* Start of environment */
    void *env_end;                  /* End of environment */
    list_head_t mmap;               /* List of memory areas */
    u32 map_count;                  /* Number of memory areas */
    u32 total_vm;                   /* Total pages mapped */
    u32 locked_vm;                  /* Pages that have PG_mlocked set */
    u32 pinned_vm;                  /* Refcount permanently increased */
    u32 data_vm;                    /* VM_WRITE & ~VM_SHARED & ~VM_STACK */
    u32 exec_vm;                    /* VM_EXEC & ~VM_WRITE & ~VM_STACK */
    u32 stack_vm;                   /* VM_STACK */
    u32 def_flags;                  /* Default VMA flags */
    u32 nr_ptes;                    /* Page table pages */
    u32 start_code_vma;             /* Start VMA for code segment */
    u32 end_code_vma;               /* End VMA for code segment */
    u32 start_data_vma;             /* Start VMA for data segment */
    u32 end_data_vma;               /* End VMA for data segment */
    u32 start_brk_vma;              /* Start VMA for heap */
    u32 brk_vma;                    /* VMA for current heap end */
    u32 start_stack_vma;            /* Start VMA for stack */
    u32 arg_vma;                    /* VMA for arguments */
    u32 env_vma;                    /* VMA for environment */
} mm_struct_t;

/* Process file structure */
typedef struct files_struct {
    u32 count;                      /* Reference count */
    u32 max_fds;                    /* Maximum number of file descriptors */
    u32 next_fd;                    /* Next free file descriptor */
    file_t **fd_array;              /* Array of file descriptors */
    u32 *close_on_exec;             /* Close on exec flags */
} files_struct_t;

/* Process signal structure */
typedef struct sighand_struct {
    u32 count;                      /* Reference count */
    u32 action[64];                 /* Signal actions */
    u32 blocked;                    /* Blocked signals */
    u32 pending;                    /* Pending signals */
    u32 saved;                      /* Saved signals */
} sighand_struct_t;

/* Process structure */
typedef struct task_struct {
    u32 pid;                        /* Process ID */
    u32 tgid;                       /* Thread group ID */
    u32 ppid;                       /* Parent process ID */
    u32 state;                      /* Process state */
    u32 flags;                      /* Process flags */
    u32 exit_code;                  /* Exit code */
    u32 exit_signal;                /* Exit signal */
    u32 pdeath_signal;              /* Parent death signal */
    u32 personality;                /* Process personality */
    u32 did_exec:1;                 /* Process has exec'd */
    u32 in_execve:1;                /* Process is doing execve */
    u32 in_iowait:1;                /* Process is waiting for I/O */
    u32 sched_reset_on_fork:1;      /* Reset scheduling on fork */
    u32 sched_contributes_to_load:1; /* Process contributes to load average */
    u32 sched_migrated:1;           /* Process was migrated */
    u32 sched_remote_wakeup:1;      /* Process was woken up remotely */
    u32 sched_cookie:1;             /* Process has a cookie */
    mm_struct_t *mm;                /* Process memory */
    mm_struct_t *active_mm;         /* Active memory */
    files_struct_t *files;          /* Process files */
    sighand_struct_t *sighand;      /* Process signals */
    security_context_t *security;   /* Process security context */
    void *stack;                    /* Kernel stack */
    void *thread;                   /* Thread information */
    char comm[16];                  /* Process name */
    struct task_struct *parent;     /* Parent process */
    struct task_struct *real_parent; /* Real parent process */
    struct task_struct *group_leader; /* Thread group leader */
    list_head_t children;           /* Child processes */
    list_head_t sibling;            /* Sibling processes */
    list_head_t tasks;              /* Task list */
    list_head_t ptraced;            /* Ptrace list */
    list_head_t ptrace_entry;       /* Ptrace entry */
    list_head_t thread_group;       /* Thread group list */
    list_head_t thread_node;        /* Thread node */
} task_struct_t;

/* Process wait options */
#define WNOHANG         0x00000001  /* Don't block waiting */
#define WUNTRACED       0x00000002  /* Report status of stopped children */
#define WSTOPPED        WUNTRACED   /* Report stopped child */
#define WEXITED         0x00000004  /* Report dead child */
#define WCONTINUED      0x00000008  /* Report continued child */
#define WNOWAIT         0x01000000  /* Don't reap, just poll status */

/* Process wait ID types */
typedef enum {
    P_ALL,      /* Wait for any child */
    P_PID,      /* Wait for specific PID */
    P_PGID      /* Wait for members of process group */
} idtype_t;

/* Resource usage who parameter */
#define RUSAGE_SELF     0       /* Current process */
#define RUSAGE_CHILDREN -1      /* All children */
#define RUSAGE_THREAD   1       /* Current thread */

/* Reboot commands */
#define LINUX_REBOOT_MAGIC1     0xfee1dead
#define LINUX_REBOOT_MAGIC2     672274793
#define LINUX_REBOOT_CMD_RESTART    0x01234567
#define LINUX_REBOOT_CMD_HALT       0xCDEF0123
#define LINUX_REBOOT_CMD_POWER_OFF  0x4321FEDC
#define LINUX_REBOOT_CMD_RESTART2   0xA1B2C3D4
#define LINUX_REBOOT_CMD_CAD_ON     0x89ABCDEF
#define LINUX_REBOOT_CMD_CAD_OFF    0x00000000

/* Kexec segment structure */
struct kexec_segment {
    void *buf;          /* Buffer in user space */
    size_t bufsz;       /* Buffer length in user space */
    void *mem;          /* Physical address */
    size_t memsz;       /* Physical address length */
};

/* User descriptor structure */
struct user_desc {
    unsigned int entry_number;   /* Entry number */
    unsigned int base_addr;      /* Base address */
    unsigned int limit;          /* Limit */
    unsigned int seg_32bit:1;    /* 32-bit segment */
    unsigned int contents:2;     /* Contents */
    unsigned int read_exec_only:1; /* Read/execute only */
    unsigned int limit_in_pages:1; /* Limit in pages */
    unsigned int seg_not_present:1; /* Segment not present */
    unsigned int useable:1;      /* Useable */
};

/* Process functions */
void process_init(void);
task_struct_t *process_create(const char *name);
int process_destroy(task_struct_t *task);
int process_exec(task_struct_t *task, const char *path, char *const argv[], char *const envp[]);
task_struct_t *process_fork(task_struct_t *parent);
int process_wait(task_struct_t *task, int *status);
int process_exit(task_struct_t *task, int status);
task_struct_t *process_current(void);
task_struct_t *process_get(u32 pid);
int process_set_state(task_struct_t *task, u32 state);
int process_set_name(task_struct_t *task, const char *name);
int process_add_file(task_struct_t *task, file_t *file);
int process_remove_file(task_struct_t *task, u32 fd);
file_t *process_get_file(task_struct_t *task, u32 fd);
int process_signal(task_struct_t *task, u32 sig);
int process_signal_group(task_struct_t *task, u32 sig);
int process_signal_all(u32 sig);

/* Process system call functions */
pid_t process_getpid(void);
pid_t process_getppid(void);
pid_t process_getpgid(pid_t pid);
int process_setpgid(pid_t pid, pid_t pgid);
pid_t process_getpgrp(void);
pid_t process_getsid(pid_t pid);
pid_t process_setsid(void);
int process_getrlimit(int resource, struct rlimit *rlim);
int process_setrlimit(int resource, const struct rlimit *rlim);
int process_prlimit64(pid_t pid, int resource, const struct rlimit64 *new_limit, struct rlimit64 *old_limit);
int process_nice(int inc);
int process_getpriority(int which, int who);
int process_setpriority(int which, int who, int prio);
int process_personality(unsigned long persona);
int process_setdomainname(const char *name, size_t len);
int process_sethostname(const char *name, size_t len);
int process_gethostname(char *name, size_t len);
int process_getdomainname(char *name, size_t len);
int process_reboot(int magic1, int magic2, int cmd, void *arg);
int process_restart_syscall(void);
int process_kexec_load(unsigned long entry, unsigned long nr_segments, struct kexec_segment *segments, unsigned long flags);
pid_t process_clone(unsigned long flags, void *stack, int *parent_tid, int *child_tid, unsigned long tls);
pid_t process_vfork(void);
pid_t process_set_tid_address(int *tidptr);
pid_t process_gettid(void);
int process_set_thread_area(struct user_desc *u_info);
int process_get_thread_area(struct user_desc *u_info);
int process_sched_getscheduler(pid_t pid);
int process_sched_setscheduler(pid_t pid, int policy, const struct sched_param *param);
int process_sched_getparam(pid_t pid, struct sched_param *param);
int process_sched_setparam(pid_t pid, const struct sched_param *param);
int process_sched_get_priority_max(int policy);
int process_sched_get_priority_min(int policy);
int process_sched_rr_get_interval(pid_t pid, struct timespec *tp);
int process_sched_yield(void);
int process_sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask);
int process_sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t *mask);
int process_getrusage(int who, struct rusage *usage);
clock_t process_times(struct tms *buf);
pid_t process_wait4(pid_t pid, int *status, int options, struct rusage *rusage);
int process_waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options, struct rusage *rusage);
int process_execve(const char *filename, char *const argv[], char *const envp[]);
void process_exit_group(int status);

#endif /* _KERNEL_PROCESS_H */
