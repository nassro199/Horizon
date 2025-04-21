/**
 * signal.h - Horizon kernel signal definitions
 *
 * This file contains definitions for signals.
 */

#ifndef _HORIZON_SIGNAL_H
#define _HORIZON_SIGNAL_H

#include <horizon/types.h>
#include <horizon/list.h>

/* Signal numbers (Linux compatible) */
#define SIGHUP           1       /* Hangup */
#define SIGINT           2       /* Interrupt */
#define SIGQUIT          3       /* Quit */
#define SIGILL           4       /* Illegal instruction */
#define SIGTRAP          5       /* Trace trap */
#define SIGABRT          6       /* Abort */
#define SIGIOT           SIGABRT /* IOT trap */
#define SIGBUS           7       /* BUS error */
#define SIGFPE           8       /* Floating-point exception */
#define SIGKILL          9       /* Kill, unblockable */
#define SIGUSR1          10      /* User-defined signal 1 */
#define SIGSEGV          11      /* Segmentation violation */
#define SIGUSR2          12      /* User-defined signal 2 */
#define SIGPIPE          13      /* Broken pipe */
#define SIGALRM          14      /* Alarm clock */
#define SIGTERM          15      /* Termination */
#define SIGSTKFLT        16      /* Stack fault */
#define SIGCHLD          17      /* Child status has changed */
#define SIGCONT          18      /* Continue */
#define SIGSTOP          19      /* Stop, unblockable */
#define SIGTSTP          20      /* Keyboard stop */
#define SIGTTIN          21      /* Background read from tty */
#define SIGTTOU          22      /* Background write to tty */
#define SIGURG           23      /* Urgent condition on socket */
#define SIGXCPU          24      /* CPU limit exceeded */
#define SIGXFSZ          25      /* File size limit exceeded */
#define SIGVTALRM        26      /* Virtual alarm clock */
#define SIGPROF          27      /* Profiling alarm clock */
#define SIGWINCH         28      /* Window size change */
#define SIGIO            29      /* I/O now possible */
#define SIGPOLL          SIGIO   /* Pollable event */
#define SIGPWR           30      /* Power failure restart */
#define SIGSYS           31      /* Bad system call */
#define SIGUNUSED        31      /* Unused signal */

/* Real-time signals */
#define SIGRTMIN  32
#define SIGRTMAX  64

/* Signal set */
typedef u64 sigset_t;

/* Signal action flags */
#define SA_NOCLDSTOP     0x00000001 /* Don't send SIGCHLD when children stop */
#define SA_NOCLDWAIT     0x00000002 /* Don't create zombie on child death */
#define SA_SIGINFO       0x00000004 /* Invoke signal-catching function with three arguments */
#define SA_ONSTACK       0x08000000 /* Use signal alternate stack */
#define SA_RESTART       0x10000000 /* Restart system call on signal return */
#define SA_NODEFER       0x40000000 /* Don't automatically block the signal when its handler is being executed */
#define SA_RESETHAND     0x80000000 /* Reset to SIG_DFL on entry to handler */
#define SA_RESTORER      0x04000000 /* Used by C libraries to restore signal mask */

/* Signal value */
typedef union sigval {
    int sival_int;              /* Integer value */
    void *sival_ptr;            /* Pointer value */
} sigval_t;

/* Signal action */
typedef struct sigaction {
    union {
        void (*sa_handler)(int);                    /* Signal handler */
        void (*sa_sigaction)(int, siginfo_t *, void *); /* Signal action */
    } _u;
    sigset_t sa_mask;           /* Signal mask to apply */
    int sa_flags;               /* Signal flags */
    void (*sa_restorer)(void);  /* Signal restorer */
} sigaction_t;

/* Signal handler */
#define sa_handler   _u.sa_handler
#define sa_sigaction _u.sa_sigaction

/* Signal information */
typedef struct siginfo {
    int si_signo;               /* Signal number */
    int si_errno;               /* Error number */
    int si_code;                /* Signal code */
    union {
        int _pad[29];           /* Padding */
        struct {
            pid_t si_pid;       /* Sending process ID */
            uid_t si_uid;       /* Real user ID of sending process */
        } _kill;
        struct {
            int si_tid;         /* Timer ID */
            int si_overrun;     /* Timer overrun count */
            sigval_t si_sigval; /* Signal value */
        } _timer;
        struct {
            pid_t si_pid;       /* Sending process ID */
            uid_t si_uid;       /* Real user ID of sending process */
            sigval_t si_sigval; /* Signal value */
        } _rt;
        struct {
            pid_t si_pid;       /* Which child */
            uid_t si_uid;       /* Real user ID of sending process */
            int si_status;      /* Exit value or signal */
            clock_t si_utime;   /* User time consumed */
            clock_t si_stime;   /* System time consumed */
        } _sigchld;
        struct {
            void *si_addr;      /* Faulting insn/memory ref. */
        } _sigfault;
        struct {
            long si_band;       /* Band event for SIGPOLL */
            int si_fd;          /* File descriptor */
        } _sigpoll;
    } _sifields;
} siginfo_t;

/* Signal stack */
typedef struct sigaltstack {
    void *ss_sp;     /* Stack base or pointer */
    int ss_flags;    /* Flags */
    size_t ss_size;  /* Stack size */
} stack_t;

/* Signal stack flags */
#define SS_ONSTACK 1  /* Currently on signal stack */
#define SS_DISABLE 2  /* Disable alternate signal stack */

/* Signal handlers */
#define SIG_DFL ((void (*)(int))0)  /* Default signal handling */
#define SIG_IGN ((void (*)(int))1)  /* Ignore signal */
#define SIG_ERR ((void (*)(int))-1) /* Error return from signal */

/* Signal codes */
#define SI_USER    0  /* Sent by kill, sigsend, raise */
#define SI_KERNEL  1  /* Sent by the kernel */
#define SI_QUEUE   2  /* Sent by sigqueue */
#define SI_TIMER   3  /* Sent by timer expiration */
#define SI_MESGQ   4  /* Sent by real time mesq state change */
#define SI_ASYNCIO 5  /* Sent by AIO completion */
#define SI_SIGIO   6  /* Sent by queued SIGIO */
#define SI_TKILL   7  /* Sent by tkill system call */
#define SI_DETHREAD 8 /* Sent by execve() killing subsidiary threads */

/* SIGILL codes */
#define ILL_ILLOPC 1  /* Illegal opcode */
#define ILL_ILLOPN 2  /* Illegal operand */
#define ILL_ILLADR 3  /* Illegal addressing mode */
#define ILL_ILLTRP 4  /* Illegal trap */
#define ILL_PRVOPC 5  /* Privileged opcode */
#define ILL_PRVREG 6  /* Privileged register */
#define ILL_COPROC 7  /* Coprocessor error */
#define ILL_BADSTK 8  /* Internal stack error */

/* SIGFPE codes */
#define FPE_INTDIV 1  /* Integer divide by zero */
#define FPE_INTOVF 2  /* Integer overflow */
#define FPE_FLTDIV 3  /* Floating-point divide by zero */
#define FPE_FLTOVF 4  /* Floating-point overflow */
#define FPE_FLTUND 5  /* Floating-point underflow */
#define FPE_FLTRES 6  /* Floating-point inexact result */
#define FPE_FLTINV 7  /* Floating-point invalid operation */
#define FPE_FLTSUB 8  /* Subscript out of range */

/* SIGSEGV codes */
#define SEGV_MAPERR 1 /* Address not mapped to object */
#define SEGV_ACCERR 2 /* Invalid permissions for mapped object */

/* SIGBUS codes */
#define BUS_ADRALN 1  /* Invalid address alignment */
#define BUS_ADRERR 2  /* Non-existent physical address */
#define BUS_OBJERR 3  /* Object-specific hardware error */

/* SIGTRAP codes */
#define TRAP_BRKPT 1  /* Process breakpoint */
#define TRAP_TRACE 2  /* Process trace trap */

/* SIGCHLD codes */
#define CLD_EXITED    1  /* Child has exited */
#define CLD_KILLED    2  /* Child was killed */
#define CLD_DUMPED    3  /* Child terminated abnormally */
#define CLD_TRAPPED   4  /* Traced child has trapped */
#define CLD_STOPPED   5  /* Child has stopped */
#define CLD_CONTINUED 6  /* Stopped child has continued */

/* SIGPOLL codes */
#define POLL_IN  1  /* Data input available */
#define POLL_OUT 2  /* Output buffers available */
#define POLL_MSG 3  /* Input message available */
#define POLL_ERR 4  /* I/O error */
#define POLL_PRI 5  /* High priority input available */
#define POLL_HUP 6  /* Device disconnected */

/* Signal pending */
typedef struct sigpending {
    struct list_head list;  /* List of pending signals */
    sigset_t signal;        /* Pending signals */
} sigpending_t;

/* Signal queue */
typedef struct sigqueue {
    struct list_head list;  /* List of queued signals */
    siginfo_t info;         /* Signal information */
} sigqueue_t;

/* Signal functions */
void signal_init(void);
int signal_setup(void);
int signal_send(struct task_struct *task, int sig);
int signal_send_info(struct task_struct *task, int sig, siginfo_t *info);
int signal_send_thread(struct thread *thread, int sig);
int signal_send_info_thread(struct thread *thread, int sig, siginfo_t *info);
int signal_queue(struct task_struct *task, int sig, siginfo_t *info);
int signal_queue_thread(struct thread *thread, int sig, siginfo_t *info);
int signal_dequeue(struct task_struct *task, siginfo_t *info);
int signal_dequeue_thread(struct thread *thread, siginfo_t *info);
int signal_pending(struct task_struct *task);
int signal_pending_thread(struct thread *thread);
int signal_do_signal(struct task_struct *task);
int signal_do_signal_thread(struct thread *thread);
int signal_handle(struct task_struct *task, int sig);
int signal_handle_thread(struct thread *thread, int sig);
int signal_mask(struct task_struct *task, int how, const sigset_t *set, sigset_t *oldset);
int signal_mask_thread(struct thread *thread, int how, const sigset_t *set, sigset_t *oldset);
int signal_action(struct task_struct *task, int sig, const struct sigaction *act, struct sigaction *oldact);
int signal_action_thread(struct thread *thread, int sig, const struct sigaction *act, struct sigaction *oldact);
int signal_wait(struct task_struct *task, const sigset_t *set, siginfo_t *info);
int signal_wait_thread(struct thread *thread, const sigset_t *set, siginfo_t *info);
int signal_timedwait(struct task_struct *task, const sigset_t *set, siginfo_t *info, const struct timespec *timeout);
int signal_timedwait_thread(struct thread *thread, const sigset_t *set, siginfo_t *info, const struct timespec *timeout);
int signal_pause(struct task_struct *task);
int signal_pause_thread(struct thread *thread);
int signal_kill(pid_t pid, int sig);
int signal_tkill(pid_t tid, int sig);
int signal_tgkill(pid_t tgid, pid_t tid, int sig);
int signal_sigqueue(pid_t pid, int sig, const union sigval value);
int signal_sigsuspend(const sigset_t *mask);
int signal_sigaction(int sig, const struct sigaction *act, struct sigaction *oldact);
int signal_sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int signal_sigpending(sigset_t *set);
int signal_sigsuspend(const sigset_t *mask);
int signal_sigwait(const sigset_t *set, int *sig);
int signal_sigwaitinfo(const sigset_t *set, siginfo_t *info);
int signal_sigtimedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout);
int signal_sigaltstack(const stack_t *ss, stack_t *old_ss);

#endif /* _HORIZON_SIGNAL_H */
