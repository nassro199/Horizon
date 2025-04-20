/**
 * syscalls.c - Horizon kernel signal system calls
 *
 * This file contains the implementation of the signal system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/signal.h>
#include <horizon/task.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: kill */
long sys_kill(long pid, long sig, long unused1, long unused2, long unused3, long unused4) {
    /* Send a signal to a process */
    return signal_kill(pid, sig);
}

/* System call: tkill */
long sys_tkill(long tid, long sig, long unused1, long unused2, long unused3, long unused4) {
    /* Send a signal to a thread */
    return signal_tkill(tid, sig);
}

/* System call: tgkill */
long sys_tgkill(long tgid, long tid, long sig, long unused1, long unused2, long unused3) {
    /* Send a signal to a specific thread in a thread group */
    return signal_tgkill(tgid, tid, sig);
}

/* System call: sigaction */
long sys_sigaction(long sig, long act, long oact, long unused1, long unused2, long unused3) {
    /* Change the action taken by a process on receipt of a specific signal */
    return signal_sigaction(sig, (const struct sigaction *)act, (struct sigaction *)oact);
}

/* System call: rt_sigaction */
long sys_rt_sigaction(long sig, long act, long oact, long sigsetsize, long unused1, long unused2) {
    /* Change the action taken by a process on receipt of a specific signal (with sigset size) */
    return signal_rt_sigaction(sig, (const struct sigaction *)act, (struct sigaction *)oact, sigsetsize);
}

/* System call: sigprocmask */
long sys_sigprocmask(long how, long set, long oset, long unused1, long unused2, long unused3) {
    /* Change the signal mask of the calling process */
    return signal_sigprocmask(how, (const sigset_t *)set, (sigset_t *)oset);
}

/* System call: rt_sigprocmask */
long sys_rt_sigprocmask(long how, long set, long oset, long sigsetsize, long unused1, long unused2) {
    /* Change the signal mask of the calling process (with sigset size) */
    return signal_rt_sigprocmask(how, (const sigset_t *)set, (sigset_t *)oset, sigsetsize);
}

/* System call: sigpending */
long sys_sigpending(long set, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Examine pending signals */
    return signal_sigpending((sigset_t *)set);
}

/* System call: rt_sigpending */
long sys_rt_sigpending(long set, long sigsetsize, long unused1, long unused2, long unused3, long unused4) {
    /* Examine pending signals (with sigset size) */
    return signal_rt_sigpending((sigset_t *)set, sigsetsize);
}

/* System call: sigsuspend */
long sys_sigsuspend(long mask, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Wait for a signal */
    return signal_sigsuspend((const sigset_t *)mask);
}

/* System call: rt_sigsuspend */
long sys_rt_sigsuspend(long mask, long sigsetsize, long unused1, long unused2, long unused3, long unused4) {
    /* Wait for a signal (with sigset size) */
    return signal_rt_sigsuspend((const sigset_t *)mask, sigsetsize);
}

/* System call: sigaltstack */
long sys_sigaltstack(long uss, long uoss, long unused1, long unused2, long unused3, long unused4) {
    /* Set and/or get signal stack context */
    return signal_sigaltstack((const stack_t *)uss, (stack_t *)uoss);
}

/* System call: rt_sigqueueinfo */
long sys_rt_sigqueueinfo(long pid, long sig, long uinfo, long unused1, long unused2, long unused3) {
    /* Queue a signal and data to a process */
    return signal_rt_sigqueueinfo(pid, sig, (siginfo_t *)uinfo);
}

/* System call: rt_sigtimedwait */
long sys_rt_sigtimedwait(long uthese, long uinfo, long uts, long sigsetsize, long unused1, long unused2) {
    /* Synchronously wait for queued signals */
    return signal_rt_sigtimedwait((const sigset_t *)uthese, (siginfo_t *)uinfo, (const struct timespec *)uts, sigsetsize);
}

/* System call: rt_sigreturn */
long sys_rt_sigreturn(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Return from signal handler and cleanup stack frame */
    return signal_rt_sigreturn();
}

/* System call: signalfd */
long sys_signalfd(long ufd, long user_mask, long sigsetsize, long unused1, long unused2, long unused3) {
    /* Create a file descriptor for accepting signals */
    return signal_signalfd(ufd, (const sigset_t *)user_mask, sigsetsize);
}

/* System call: signalfd4 */
long sys_signalfd4(long ufd, long user_mask, long sigsetsize, long flags, long unused1, long unused2) {
    /* Create a file descriptor for accepting signals */
    return signal_signalfd4(ufd, (const sigset_t *)user_mask, sigsetsize, flags);
}

/* System call: sigreturn */
long sys_sigreturn(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Return from signal handler and cleanup stack frame */
    return signal_sigreturn();
}

/* System call: sigwaitinfo */
long sys_sigwaitinfo(long uthese, long uinfo, long unused1, long unused2, long unused3, long unused4) {
    /* Synchronously wait for queued signals */
    return signal_sigwaitinfo((const sigset_t *)uthese, (siginfo_t *)uinfo);
}

/* System call: sigtimedwait */
long sys_sigtimedwait(long uthese, long uinfo, long uts, long unused1, long unused2, long unused3) {
    /* Synchronously wait for queued signals */
    return signal_sigtimedwait((const sigset_t *)uthese, (siginfo_t *)uinfo, (const struct timespec *)uts);
}

/* System call: pause */
long sys_pause(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Wait for signal */
    return signal_pause();
}

/* Register signal system calls */
void signal_syscalls_init(void) {
    /* Register signal system calls */
    syscall_register(SYS_KILL, sys_kill);
    syscall_register(SYS_TKILL, sys_tkill);
    syscall_register(SYS_TGKILL, sys_tgkill);
    syscall_register(SYS_SIGACTION, sys_sigaction);
    syscall_register(SYS_RT_SIGACTION, sys_rt_sigaction);
    syscall_register(SYS_SIGPROCMASK, sys_sigprocmask);
    syscall_register(SYS_RT_SIGPROCMASK, sys_rt_sigprocmask);
    syscall_register(SYS_SIGPENDING, sys_sigpending);
    syscall_register(SYS_RT_SIGPENDING, sys_rt_sigpending);
    syscall_register(SYS_SIGSUSPEND, sys_sigsuspend);
    syscall_register(SYS_RT_SIGSUSPEND, sys_rt_sigsuspend);
    syscall_register(SYS_SIGALTSTACK, sys_sigaltstack);
    syscall_register(SYS_RT_SIGQUEUEINFO, sys_rt_sigqueueinfo);
    syscall_register(SYS_RT_SIGTIMEDWAIT, sys_rt_sigtimedwait);
    syscall_register(SYS_RT_SIGRETURN, sys_rt_sigreturn);
    syscall_register(SYS_SIGNALFD, sys_signalfd);
    syscall_register(SYS_SIGNALFD4, sys_signalfd4);
    syscall_register(SYS_SIGRETURN, sys_sigreturn);
    syscall_register(SYS_SIGWAITINFO, sys_sigwaitinfo);
    syscall_register(SYS_SIGTIMEDWAIT, sys_sigtimedwait);
    syscall_register(SYS_PAUSE, sys_pause);
}
