/**
 * process.c - Horizon kernel process-related system calls
 *
 * This file contains the implementation of process-related system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/sched.h>
#include <horizon/task.h>
#include <horizon/thread.h>
#include <horizon/mm.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Exit system call */
long sys_exit(long status, long arg2, long arg3, long arg4, long arg5, long arg6) {
    sched_exit((int)status);
    return 0;
}

/* Fork system call */
long sys_fork(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Create a new process */
    struct task_struct *child = sched_fork();
    if (child == NULL) {
        return -ENOMEM;
    }

    /* Return the child's PID to the parent */
    return child->pid;
}

/* Exec system call */
long sys_execve(long path, long argv, long envp, long arg4, long arg5, long arg6) {
    /* Execute a new program */
    return sched_exec((const char *)path, (char *const *)argv, (char *const *)envp);
}

/* Wait system call */
long sys_wait4(long pid, long status, long options, long rusage, long arg5, long arg6) {
    /* Wait for a process to change state */
    return sched_wait(pid, (int *)status, options, (struct rusage *)rusage);
}

/* Get process ID system call */
long sys_getpid(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Return the current process ID */
    return task_current()->pid;
}

/* Get parent process ID system call */
long sys_getppid(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Return the parent process ID */
    return task_current()->parent->pid;
}

/* Get thread ID system call */
long sys_gettid(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Return the current thread ID */
    return thread_self()->tid;
}

/* Set thread ID address system call */
long sys_set_tid_address(long tidptr, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Set the clear child TID address */
    thread_self()->clear_child_tid = (int *)tidptr;
    return thread_self()->tid;
}

/* Clone system call */
long sys_clone(long flags, long stack, long parent_tidptr, long child_tidptr, long tls, long arg6) {
    /* Create a new thread or process */
    return sched_clone(flags, (void *)stack, (int *)parent_tidptr, (int *)child_tidptr, (void *)tls);
}

/* Exit group system call */
long sys_exit_group(long status, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Exit all threads in the current thread group */
    sched_exit_group((int)status);
    return 0;
}

/* Get process group ID system call */
long sys_getpgid(long pid, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Get the process group ID of the specified process */
    struct task_struct *task;
    
    if (pid == 0) {
        task = task_current();
    } else {
        task = task_get(pid);
        if (task == NULL) {
            return -ESRCH;
        }
    }
    
    return task->pgid;
}

/* Set process group ID system call */
long sys_setpgid(long pid, long pgid, long arg3, long arg4, long arg5, long arg6) {
    /* Set the process group ID of the specified process */
    struct task_struct *task;
    
    if (pid == 0) {
        task = task_current();
    } else {
        task = task_get(pid);
        if (task == NULL) {
            return -ESRCH;
        }
    }
    
    if (pgid == 0) {
        pgid = task->pid;
    }
    
    task->pgid = pgid;
    return 0;
}

/* Get process group system call */
long sys_getpgrp(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Get the process group ID of the current process */
    return task_current()->pgid;
}

/* Get session ID system call */
long sys_getsid(long pid, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Get the session ID of the specified process */
    struct task_struct *task;
    
    if (pid == 0) {
        task = task_current();
    } else {
        task = task_get(pid);
        if (task == NULL) {
            return -ESRCH;
        }
    }
    
    return task->sid;
}

/* Create a new session system call */
long sys_setsid(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Create a new session */
    struct task_struct *task = task_current();
    
    /* Check if the process is already a process group leader */
    if (task->pid == task->pgid) {
        return -EPERM;
    }
    
    /* Create a new session */
    task->sid = task->pid;
    task->pgid = task->pid;
    
    return task->sid;
}

/* Initialize process-related system calls */
void process_syscalls_init(void) {
    /* Register process-related system calls */
    syscall_register(SYS_EXIT, sys_exit);
    syscall_register(SYS_FORK, sys_fork);
    syscall_register(SYS_EXECVE, sys_execve);
    syscall_register(SYS_WAIT4, sys_wait4);
    syscall_register(SYS_GETPID, sys_getpid);
    syscall_register(SYS_GETPPID, sys_getppid);
    syscall_register(SYS_GETTID, sys_gettid);
    syscall_register(SYS_SET_TID_ADDRESS, sys_set_tid_address);
    syscall_register(SYS_CLONE, sys_clone);
    syscall_register(SYS_EXIT_GROUP, sys_exit_group);
    syscall_register(SYS_GETPGID, sys_getpgid);
    syscall_register(SYS_SETPGID, sys_setpgid);
    syscall_register(SYS_GETPGRP, sys_getpgrp);
    syscall_register(SYS_GETSID, sys_getsid);
    syscall_register(SYS_SETSID, sys_setsid);
}
