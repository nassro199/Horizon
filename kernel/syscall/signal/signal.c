/**
 * signal.c - Horizon kernel signal-related system calls
 *
 * This file contains the implementation of signal-related system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/signal.h>
#include <horizon/task.h>
#include <horizon/thread.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Kill system call */
long sys_kill(long pid, long sig, long arg3, long arg4, long arg5, long arg6) {
    /* Send a signal to a process */
    return signal_kill(pid, sig);
}

/* Thread kill system call */
long sys_tkill(long tid, long sig, long arg3, long arg4, long arg5, long arg6) {
    /* Send a signal to a thread */
    return signal_tkill(tid, sig);
}

/* Thread group kill system call */
long sys_tgkill(long tgid, long tid, long sig, long arg4, long arg5, long arg6) {
    /* Send a signal to a specific thread in a thread group */
    return signal_tgkill(tgid, tid, sig);
}

/* Signal action system call */
long sys_sigaction(long sig, long act, long oact, long arg4, long arg5, long arg6) {
    /* Change the action taken by a process on receipt of a specific signal */
    return signal_sigaction(sig, (const struct sigaction *)act, (struct sigaction *)oact);
}

/* Real-time signal action system call */
long sys_rt_sigaction(long sig, long act, long oact, long sigsetsize, long arg5, long arg6) {
    /* Change the action taken by a process on receipt of a specific signal (with sigset size) */
    return signal_rt_sigaction(sig, (const struct sigaction *)act, (struct sigaction *)oact, sigsetsize);
}

/* Signal process mask system call */
long sys_sigprocmask(long how, long set, long oldset, long arg4, long arg5, long arg6) {
    /* Change the signal mask of the calling process */
    return signal_sigprocmask(how, (const sigset_t *)set, (sigset_t *)oldset);
}

/* Real-time signal process mask system call */
long sys_rt_sigprocmask(long how, long set, long oldset, long sigsetsize, long arg5, long arg6) {
    /* Change the signal mask of the calling process (with sigset size) */
    return signal_rt_sigprocmask(how, (const sigset_t *)set, (sigset_t *)oldset, sigsetsize);
}

/* Signal pending system call */
long sys_sigpending(long set, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Examine pending signals */
    return signal_sigpending((sigset_t *)set);
}

/* Real-time signal pending system call */
long sys_rt_sigpending(long set, long sigsetsize, long arg3, long arg4, long arg5, long arg6) {
    /* Examine pending signals (with sigset size) */
    return signal_rt_sigpending((sigset_t *)set, sigsetsize);
}

/* Signal suspend system call */
long sys_sigsuspend(long mask, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Wait for a signal */
    return signal_sigsuspend((const sigset_t *)mask);
}

/* Real-time signal suspend system call */
long sys_rt_sigsuspend(long mask, long sigsetsize, long arg3, long arg4, long arg5, long arg6) {
    /* Wait for a signal (with sigset size) */
    return signal_rt_sigsuspend((const sigset_t *)mask, sigsetsize);
}

/* Signal wait info system call */
long sys_sigwaitinfo(long uthese, long uinfo, long arg3, long arg4, long arg5, long arg6) {
    /* Synchronously wait for queued signals */
    return signal_sigwaitinfo((const sigset_t *)uthese, (siginfo_t *)uinfo);
}

/* Real-time signal wait info system call */
long sys_rt_sigtimedwait(long uthese, long uinfo, long uts, long sigsetsize, long arg5, long arg6) {
    /* Synchronously wait for queued signals (with timeout and sigset size) */
    return signal_rt_sigtimedwait((const sigset_t *)uthese, (siginfo_t *)uinfo, (const struct timespec *)uts, sigsetsize);
}

/* Real-time signal queue info system call */
long sys_rt_sigqueueinfo(long pid, long sig, long uinfo, long arg4, long arg5, long arg6) {
    /* Queue a signal and data to a process */
    return signal_rt_sigqueueinfo(pid, sig, (siginfo_t *)uinfo);
}

/* Signal return system call */
long sys_sigreturn(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Return from signal handler and cleanup stack frame */
    return signal_sigreturn();
}

/* Real-time signal return system call */
long sys_rt_sigreturn(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Return from signal handler and cleanup stack frame (with rt prefix) */
    return signal_rt_sigreturn();
}

/* Pause system call */
long sys_pause(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Wait for signal */
    return signal_pause();
}

/* Signal alternate stack system call */
long sys_sigaltstack(long ss, long old_ss, long arg3, long arg4, long arg5, long arg6) {
    /* Set and/or get signal stack context */
    return signal_sigaltstack((const stack_t *)ss, (stack_t *)old_ss);
}

/* Initialize signal-related system calls */
void signal_syscalls_init(void) {
    /* Register signal-related system calls */
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
    syscall_register(SYS_SIGWAITINFO, sys_sigwaitinfo);
    syscall_register(SYS_RT_SIGTIMEDWAIT, sys_rt_sigtimedwait);
    syscall_register(SYS_RT_SIGQUEUEINFO, sys_rt_sigqueueinfo);
    syscall_register(SYS_SIGRETURN, sys_sigreturn);
    syscall_register(SYS_RT_SIGRETURN, sys_rt_sigreturn);
    syscall_register(SYS_PAUSE, sys_pause);
    syscall_register(SYS_SIGALTSTACK, sys_sigaltstack);
}
