/**
 * syscalls.c - Horizon kernel process control system calls
 *
 * This file contains the implementation of the process control system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/process.h>
#include <horizon/task.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: fork */
long sys_fork(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Create a child process */
    return process_fork();
}

/* System call: vfork */
long sys_vfork(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Create a child process and block parent */
    return process_vfork();
}

/* System call: clone */
long sys_clone(long flags, long stack, long parent_tid, long child_tid, long tls, long unused1) {
    /* Create a child process with more control */
    return process_clone(flags, (void *)stack, (int *)parent_tid, (int *)child_tid, (unsigned long)tls);
}

/* System call: execve */
long sys_execve(long filename, long argv, long envp, long unused1, long unused2, long unused3) {
    /* Execute program */
    return process_execve((const char *)filename, (char *const *)argv, (char *const *)envp);
}

/* System call: exit */
long sys_exit(long status, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Terminate the calling process */
    process_exit(status);
    return 0; /* Never reached */
}

/* System call: exit_group */
long sys_exit_group(long status, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Terminate all threads in a process */
    process_exit_group(status);
    return 0; /* Never reached */
}

/* System call: wait4 */
long sys_wait4(long pid, long status, long options, long rusage, long unused1, long unused2) {
    /* Wait for process to change state */
    return process_wait4(pid, (int *)status, options, (struct rusage *)rusage);
}

/* System call: waitid */
long sys_waitid(long idtype, long id, long infop, long options, long rusage, long unused1) {
    /* Wait for process to change state */
    return process_waitid(idtype, id, (siginfo_t *)infop, options, (struct rusage *)rusage);
}

/* System call: getpid */
long sys_getpid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Get process identification */
    return process_getpid();
}

/* System call: getppid */
long sys_getppid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Get parent process identification */
    return process_getppid();
}

/* System call: getpgid */
long sys_getpgid(long pid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get process group ID for a process */
    return process_getpgid(pid);
}

/* System call: setpgid */
long sys_setpgid(long pid, long pgid, long unused1, long unused2, long unused3, long unused4) {
    /* Set process group ID for process */
    return process_setpgid(pid, pgid);
}

/* System call: getpgrp */
long sys_getpgrp(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Get process group ID of the calling process */
    return process_getpgrp();
}

/* System call: getsid */
long sys_getsid(long pid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get session ID */
    return process_getsid(pid);
}

/* System call: setsid */
long sys_setsid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Create session and set process group ID */
    return process_setsid();
}

/* System call: getrlimit */
long sys_getrlimit(long resource, long rlim, long unused1, long unused2, long unused3, long unused4) {
    /* Get resource limits */
    return process_getrlimit(resource, (struct rlimit *)rlim);
}

/* System call: setrlimit */
long sys_setrlimit(long resource, long rlim, long unused1, long unused2, long unused3, long unused4) {
    /* Set resource limits */
    return process_setrlimit(resource, (const struct rlimit *)rlim);
}

/* System call: prlimit64 */
long sys_prlimit64(long pid, long resource, long new_limit, long old_limit, long unused1, long unused2) {
    /* Get/set resource limits */
    return process_prlimit64(pid, resource, (const struct rlimit64 *)new_limit, (struct rlimit64 *)old_limit);
}

/* System call: nice */
long sys_nice(long inc, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Change process priority */
    return process_nice(inc);
}

/* System call: getpriority */
long sys_getpriority(long which, long who, long unused1, long unused2, long unused3, long unused4) {
    /* Get program scheduling priority */
    return process_getpriority(which, who);
}

/* System call: setpriority */
long sys_setpriority(long which, long who, long prio, long unused1, long unused2, long unused3) {
    /* Set program scheduling priority */
    return process_setpriority(which, who, prio);
}

/* System call: personality */
long sys_personality(long persona, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Set the process execution domain */
    return process_personality(persona);
}

/* System call: setdomainname */
long sys_setdomainname(long name, long len, long unused1, long unused2, long unused3, long unused4) {
    /* Set NIS domain name */
    return process_setdomainname((const char *)name, len);
}

/* System call: sethostname */
long sys_sethostname(long name, long len, long unused1, long unused2, long unused3, long unused4) {
    /* Set hostname */
    return process_sethostname((const char *)name, len);
}

/* System call: gethostname */
long sys_gethostname(long name, long len, long unused1, long unused2, long unused3, long unused4) {
    /* Get hostname */
    return process_gethostname((char *)name, len);
}

/* System call: getdomainname */
long sys_getdomainname(long name, long len, long unused1, long unused2, long unused3, long unused4) {
    /* Get NIS domain name */
    return process_getdomainname((char *)name, len);
}

/* System call: reboot */
long sys_reboot(long magic1, long magic2, long cmd, long arg, long unused1, long unused2) {
    /* Reboot or enable/disable Ctrl-Alt-Del */
    return process_reboot(magic1, magic2, cmd, arg);
}

/* System call: restart_syscall */
long sys_restart_syscall(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Restart a system call after interruption by a stop signal */
    return process_restart_syscall();
}

/* System call: kexec_load */
long sys_kexec_load(long entry, long nr_segments, long segments, long flags, long unused1, long unused2) {
    /* Load a new kernel for later execution */
    return process_kexec_load(entry, nr_segments, (struct kexec_segment *)segments, flags);
}

/* System call: set_tid_address */
long sys_set_tid_address(long tidptr, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Set pointer to thread ID */
    return process_set_tid_address((int *)tidptr);
}

/* System call: gettid */
long sys_gettid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Get thread identification */
    return process_gettid();
}

/* System call: set_thread_area */
long sys_set_thread_area(long u_info, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Set a thread local storage (TLS) area */
    return process_set_thread_area((struct user_desc *)u_info);
}

/* System call: get_thread_area */
long sys_get_thread_area(long u_info, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get a thread local storage (TLS) area */
    return process_get_thread_area((struct user_desc *)u_info);
}

/* System call: sched_getscheduler */
long sys_sched_getscheduler(long pid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get scheduling policy */
    return process_sched_getscheduler(pid);
}

/* System call: sched_setscheduler */
long sys_sched_setscheduler(long pid, long policy, long param, long unused1, long unused2, long unused3) {
    /* Set scheduling policy and parameters */
    return process_sched_setscheduler(pid, policy, (const struct sched_param *)param);
}

/* System call: sched_getparam */
long sys_sched_getparam(long pid, long param, long unused1, long unused2, long unused3, long unused4) {
    /* Get scheduling parameters */
    return process_sched_getparam(pid, (struct sched_param *)param);
}

/* System call: sched_setparam */
long sys_sched_setparam(long pid, long param, long unused1, long unused2, long unused3, long unused4) {
    /* Set scheduling parameters */
    return process_sched_setparam(pid, (const struct sched_param *)param);
}

/* System call: sched_get_priority_max */
long sys_sched_get_priority_max(long policy, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get maximum scheduling priority */
    return process_sched_get_priority_max(policy);
}

/* System call: sched_get_priority_min */
long sys_sched_get_priority_min(long policy, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get minimum scheduling priority */
    return process_sched_get_priority_min(policy);
}

/* System call: sched_rr_get_interval */
long sys_sched_rr_get_interval(long pid, long interval, long unused1, long unused2, long unused3, long unused4) {
    /* Get round-robin time quantum */
    return process_sched_rr_get_interval(pid, (struct timespec *)interval);
}

/* System call: sched_yield */
long sys_sched_yield(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Yield the processor */
    return process_sched_yield();
}

/* System call: sched_getaffinity */
long sys_sched_getaffinity(long pid, long len, long mask, long unused1, long unused2, long unused3) {
    /* Get CPU affinity mask */
    return process_sched_getaffinity(pid, len, (cpu_set_t *)mask);
}

/* System call: sched_setaffinity */
long sys_sched_setaffinity(long pid, long len, long mask, long unused1, long unused2, long unused3) {
    /* Set CPU affinity mask */
    return process_sched_setaffinity(pid, len, (const cpu_set_t *)mask);
}

/* System call: getrusage */
long sys_getrusage(long who, long usage, long unused1, long unused2, long unused3, long unused4) {
    /* Get resource usage */
    return process_getrusage(who, (struct rusage *)usage);
}

/* System call: times */
long sys_times(long buf, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get process times */
    return process_times((struct tms *)buf);
}

/* Register process control system calls */
void process_syscalls_init(void) {
    /* Register process control system calls */
    syscall_register(SYS_FORK, sys_fork);
    syscall_register(SYS_VFORK, sys_vfork);
    syscall_register(SYS_CLONE, sys_clone);
    syscall_register(SYS_EXECVE, sys_execve);
    syscall_register(SYS_EXIT, sys_exit);
    syscall_register(SYS_EXIT_GROUP, sys_exit_group);
    syscall_register(SYS_WAIT4, sys_wait4);
    syscall_register(SYS_WAITID, sys_waitid);
    syscall_register(SYS_GETPID, sys_getpid);
    syscall_register(SYS_GETPPID, sys_getppid);
    syscall_register(SYS_GETPGID, sys_getpgid);
    syscall_register(SYS_SETPGID, sys_setpgid);
    syscall_register(SYS_GETPGRP, sys_getpgrp);
    syscall_register(SYS_GETSID, sys_getsid);
    syscall_register(SYS_SETSID, sys_setsid);
    syscall_register(SYS_GETRLIMIT, sys_getrlimit);
    syscall_register(SYS_SETRLIMIT, sys_setrlimit);
    syscall_register(SYS_PRLIMIT64, sys_prlimit64);
    syscall_register(SYS_NICE, sys_nice);
    syscall_register(SYS_GETPRIORITY, sys_getpriority);
    syscall_register(SYS_SETPRIORITY, sys_setpriority);
    syscall_register(SYS_PERSONALITY, sys_personality);
    syscall_register(SYS_SETDOMAINNAME, sys_setdomainname);
    syscall_register(SYS_SETHOSTNAME, sys_sethostname);
    syscall_register(SYS_GETHOSTNAME, sys_gethostname);
    syscall_register(SYS_GETDOMAINNAME, sys_getdomainname);
    syscall_register(SYS_REBOOT, sys_reboot);
    syscall_register(SYS_RESTART_SYSCALL, sys_restart_syscall);
    syscall_register(SYS_KEXEC_LOAD, sys_kexec_load);
    syscall_register(SYS_SET_TID_ADDRESS, sys_set_tid_address);
    syscall_register(SYS_GETTID, sys_gettid);
    syscall_register(SYS_SET_THREAD_AREA, sys_set_thread_area);
    syscall_register(SYS_GET_THREAD_AREA, sys_get_thread_area);
    syscall_register(SYS_SCHED_GETSCHEDULER, sys_sched_getscheduler);
    syscall_register(SYS_SCHED_SETSCHEDULER, sys_sched_setscheduler);
    syscall_register(SYS_SCHED_GETPARAM, sys_sched_getparam);
    syscall_register(SYS_SCHED_SETPARAM, sys_sched_setparam);
    syscall_register(SYS_SCHED_GET_PRIORITY_MAX, sys_sched_get_priority_max);
    syscall_register(SYS_SCHED_GET_PRIORITY_MIN, sys_sched_get_priority_min);
    syscall_register(SYS_SCHED_RR_GET_INTERVAL, sys_sched_rr_get_interval);
    syscall_register(SYS_SCHED_YIELD, sys_sched_yield);
    syscall_register(SYS_SCHED_GETAFFINITY, sys_sched_getaffinity);
    syscall_register(SYS_SCHED_SETAFFINITY, sys_sched_setaffinity);
    syscall_register(SYS_GETRUSAGE, sys_getrusage);
    syscall_register(SYS_TIMES, sys_times);
}
