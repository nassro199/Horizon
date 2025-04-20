/**
 * signal.h - Horizon kernel signal definitions
 *
 * This file contains definitions for the signal handling subsystem.
 */

#ifndef _KERNEL_SIGNAL_H
#define _KERNEL_SIGNAL_H

#include <horizon/types.h>

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

/* Signal set size */
#define _NSIG            64
#define _NSIG_BPW        (sizeof(unsigned long) * 8)
#define _NSIG_WORDS      (_NSIG / _NSIG_BPW)

/* Signal set type */
typedef struct {
    unsigned long sig[_NSIG_WORDS];
} sigset_t;

/* Signal action flags */
#define SA_NOCLDSTOP     0x00000001 /* Don't send SIGCHLD when children stop */
#define SA_NOCLDWAIT     0x00000002 /* Don't create zombie on child death */
#define SA_SIGINFO       0x00000004 /* Invoke signal-catching function with three arguments */
#define SA_ONSTACK       0x08000000 /* Use signal alternate stack */
#define SA_RESTART       0x10000000 /* Restart system call on signal return */
#define SA_NODEFER       0x40000000 /* Don't automatically block the signal when its handler is being executed */
#define SA_RESETHAND     0x80000000 /* Reset to SIG_DFL on entry to handler */
#define SA_RESTORER      0x04000000 /* Used by C libraries to restore signal mask */

/* Signal action structure */
struct sigaction {
    void (*sa_handler)(int);                     /* Signal handler */
    void (*sa_sigaction)(int, siginfo_t *, void *); /* Signal action */
    sigset_t sa_mask;                           /* Signal mask to apply */
    int sa_flags;                               /* Signal flags */
    void (*sa_restorer)(void);                  /* Signal restorer */
};

/* Signal information structure */
typedef struct {
    int si_signo;                               /* Signal number */
    int si_errno;                               /* Error number */
    int si_code;                                /* Signal code */
    union {
        int _pad[29];                           /* Padding */
        /* Kill */
        struct {
            pid_t si_pid;                       /* Sending process ID */
            uid_t si_uid;                       /* Real user ID of sending process */
        } _kill;
        /* POSIX.1b timers */
        struct {
            int si_tid;                         /* Timer ID */
            int si_overrun;                     /* Timer overrun count */
            sigval_t si_sigval;                 /* Signal value */
        } _timer;
        /* POSIX.1b signals */
        struct {
            pid_t si_pid;                       /* Sending process ID */
            uid_t si_uid;                       /* Real user ID of sending process */
            sigval_t si_sigval;                 /* Signal value */
        } _rt;
        /* SIGCHLD */
        struct {
            pid_t si_pid;                       /* Which child */
            uid_t si_uid;                       /* Real user ID of sending process */
            int si_status;                      /* Exit value or signal */
            clock_t si_utime;                   /* User time consumed */
            clock_t si_stime;                   /* System time consumed */
        } _sigchld;
        /* SIGILL, SIGFPE, SIGSEGV, SIGBUS */
        struct {
            void *si_addr;                      /* Faulting insn/memory ref. */
        } _sigfault;
        /* SIGPOLL */
        struct {
            long si_band;                       /* Band event for SIGPOLL */
            int si_fd;                          /* File descriptor */
        } _sigpoll;
    } _sifields;
} siginfo_t;

/* Signal stack structure */
typedef struct {
    void *ss_sp;                                /* Stack base or pointer */
    int ss_flags;                               /* Flags */
    size_t ss_size;                             /* Stack size */
} stack_t;

/* Signal stack flags */
#define SS_ONSTACK       1                      /* Currently on signal stack */
#define SS_DISABLE       2                      /* Disable taking signals on alternate stack */

/* Signal file descriptor structure */
struct signalfd_siginfo {
    u32 ssi_signo;                              /* Signal number */
    s32 ssi_errno;                              /* Error number */
    s32 ssi_code;                               /* Signal code */
    u32 ssi_pid;                                /* PID of sender */
    u32 ssi_uid;                                /* Real UID of sender */
    s32 ssi_fd;                                 /* File descriptor */
    u32 ssi_tid;                                /* Kernel timer ID */
    u32 ssi_band;                               /* Band event */
    u32 ssi_overrun;                            /* Timer overrun count */
    u32 ssi_trapno;                             /* Trap number */
    s32 ssi_status;                             /* Exit status or signal */
    s32 ssi_int;                                /* Integer sent by sigqueue */
    u64 ssi_ptr;                                /* Pointer sent by sigqueue */
    u64 ssi_utime;                              /* User CPU time consumed */
    u64 ssi_stime;                              /* System CPU time consumed */
    u64 ssi_addr;                               /* Address that generated signal */
    u8 __pad[128 - 12 * sizeof(u32) - 4 * sizeof(u64)]; /* Pad to 128 bytes */
};

/* Signal functions */
int signal_kill(pid_t pid, int sig);
int signal_tkill(pid_t tid, int sig);
int signal_tgkill(pid_t tgid, pid_t tid, int sig);
int signal_sigaction(int sig, const struct sigaction *act, struct sigaction *oact);
int signal_rt_sigaction(int sig, const struct sigaction *act, struct sigaction *oact, size_t sigsetsize);
int signal_sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int signal_rt_sigprocmask(int how, const sigset_t *set, sigset_t *oldset, size_t sigsetsize);
int signal_sigpending(sigset_t *set);
int signal_rt_sigpending(sigset_t *set, size_t sigsetsize);
int signal_sigsuspend(const sigset_t *mask);
int signal_rt_sigsuspend(const sigset_t *mask, size_t sigsetsize);
int signal_sigaltstack(const stack_t *ss, stack_t *oss);
int signal_rt_sigqueueinfo(pid_t pid, int sig, siginfo_t *uinfo);
int signal_rt_sigtimedwait(const sigset_t *uthese, siginfo_t *uinfo, const struct timespec *uts, size_t sigsetsize);
int signal_rt_sigreturn(void);
int signal_signalfd(int ufd, const sigset_t *user_mask, size_t sigsetsize);
int signal_signalfd4(int ufd, const sigset_t *user_mask, size_t sigsetsize, int flags);
int signal_sigreturn(void);
int signal_sigwaitinfo(const sigset_t *uthese, siginfo_t *uinfo);
int signal_sigtimedwait(const sigset_t *uthese, siginfo_t *uinfo, const struct timespec *uts);
int signal_pause(void);
void signal_get_pending(sigset_t *set);
void signal_clear_pending(int sig);

#endif /* _KERNEL_SIGNAL_H */
