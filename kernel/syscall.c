/**
 * syscall.c - Horizon kernel system call implementation
 *
 * This file contains the implementation of the system call interface.
 * The system call interface is compatible with Linux for x86.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/sched.h>
#include <horizon/fs.h>
#include <horizon/mm.h>
#include <horizon/vmm.h>
#include <horizon/time.h>
#include <horizon/net.h>
#include <horizon/string.h>
#include <horizon/task.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Define error codes if not defined */
#ifndef EBADF
#define EBADF 9       /* Bad file descriptor */
#endif

#ifndef EINVAL
#define EINVAL 22     /* Invalid argument */
#endif

#ifndef ENOSYS
#define ENOSYS 38     /* Function not implemented */
#endif

/* System call table */
syscall_handler_t syscall_table[MAX_SYSCALLS];

/* System call handlers - defined in kernel/syscall/ subdirectories */

/* Process-related system calls - defined in kernel/syscall/process/process.c */
extern long sys_exit(long status, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_fork(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_execve(long path, long argv, long envp, long arg4, long arg5, long arg6);
extern long sys_wait4(long pid, long status, long options, long rusage, long arg5, long arg6);
extern long sys_getpid(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_getppid(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_gettid(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_set_tid_address(long tidptr, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_clone(long flags, long stack, long parent_tidptr, long child_tidptr, long tls, long arg6);
extern long sys_exit_group(long status, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_getpgid(long pid, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_setpgid(long pid, long pgid, long arg3, long arg4, long arg5, long arg6);
extern long sys_getpgrp(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_getsid(long pid, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_setsid(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);

/* File system-related system calls - defined in kernel/syscall/fs/fs.c */
extern long sys_read(long fd, long buffer, long size, long arg4, long arg5, long arg6);
extern long sys_write(long fd, long buffer, long size, long arg4, long arg5, long arg6);
extern long sys_open(long pathname, long flags, long mode, long unused1, long unused2, long unused3);
extern long sys_openat(long dirfd, long pathname, long flags, long mode, long unused1, long unused2);
extern long sys_creat(long pathname, long mode, long unused1, long unused2, long unused3, long unused4);
extern long sys_close(long fd, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_lseek(long fd, long offset, long whence, long arg4, long arg5, long arg6);
extern long sys_dup(long fd, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_dup2(long oldfd, long newfd, long arg3, long arg4, long arg5, long arg6);
extern long sys_pipe(long pipefd, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_pipe2(long pipefd, long flags, long arg3, long arg4, long arg5, long arg6);
extern long sys_fcntl(long fd, long cmd, long arg, long arg4, long arg5, long arg6);
extern long sys_ioctl(long fd, long cmd, long arg, long arg4, long arg5, long arg6);

/* Memory management-related system calls - defined in kernel/syscall/mm/mm.c */
extern long sys_mmap(long addr, long length, long prot, long flags, long fd, long offset);
extern long sys_munmap(long addr, long length, long arg3, long arg4, long arg5, long arg6);
extern long sys_brk(long brk, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_mprotect(long addr, long len, long prot, long arg4, long arg5, long arg6);
extern long sys_mremap(long old_address, long old_size, long new_size, long flags, long new_address, long arg6);
extern long sys_mlock(long addr, long len, long arg3, long arg4, long arg5, long arg6);
extern long sys_munlock(long addr, long len, long arg3, long arg4, long arg5, long arg6);
extern long sys_mlockall(long flags, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_munlockall(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_madvise(long addr, long length, long advice, long arg4, long arg5, long arg6);
extern long sys_msync(long addr, long length, long flags, long arg4, long arg5, long arg6);
extern long sys_mincore(long addr, long length, long vec, long arg4, long arg5, long arg6);

/* Time-related system calls - defined in kernel/syscall/time/time.c */
extern long sys_gettimeofday(long tv, long tz, long arg3, long arg4, long arg5, long arg6);
extern long sys_settimeofday(long tv, long tz, long arg3, long arg4, long arg5, long arg6);
extern long sys_nanosleep(long req, long rem, long arg3, long arg4, long arg5, long arg6);
extern long sys_time(long tloc, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_clock_gettime(long clockid, long tp, long arg3, long arg4, long arg5, long arg6);
extern long sys_clock_settime(long clockid, long tp, long arg3, long arg4, long arg5, long arg6);
extern long sys_clock_getres(long clockid, long res, long arg3, long arg4, long arg5, long arg6);
extern long sys_clock_nanosleep(long clockid, long flags, long req, long rem, long arg5, long arg6);

/* Signal-related system calls - defined in kernel/syscall/signal/signal.c */
extern long sys_kill(long pid, long sig, long arg3, long arg4, long arg5, long arg6);
extern long sys_tkill(long tid, long sig, long arg3, long arg4, long arg5, long arg6);
extern long sys_tgkill(long tgid, long tid, long sig, long arg4, long arg5, long arg6);
extern long sys_sigaction(long sig, long act, long oact, long arg4, long arg5, long arg6);
extern long sys_rt_sigaction(long sig, long act, long oact, long sigsetsize, long arg5, long arg6);
extern long sys_sigprocmask(long how, long set, long oldset, long arg4, long arg5, long arg6);
extern long sys_rt_sigprocmask(long how, long set, long oldset, long sigsetsize, long arg5, long arg6);
extern long sys_sigpending(long set, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_rt_sigpending(long set, long sigsetsize, long arg3, long arg4, long arg5, long arg6);
extern long sys_sigsuspend(long mask, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_rt_sigsuspend(long mask, long sigsetsize, long arg3, long arg4, long arg5, long arg6);
extern long sys_sigwaitinfo(long uthese, long uinfo, long arg3, long arg4, long arg5, long arg6);
extern long sys_rt_sigtimedwait(long uthese, long uinfo, long uts, long sigsetsize, long arg5, long arg6);
extern long sys_rt_sigqueueinfo(long pid, long sig, long uinfo, long arg4, long arg5, long arg6);
extern long sys_sigreturn(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_rt_sigreturn(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_pause(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_sigaltstack(long ss, long old_ss, long arg3, long arg4, long arg5, long arg6);

/* IPC-related system calls - defined in kernel/syscall/ipc/ipc.c */
extern long sys_ipc(long call, long first, long second, long third, long ptr, long fifth);
extern long sys_semget(long key, long nsems, long semflg, long arg4, long arg5, long arg6);
extern long sys_semop(long semid, long sops, long nsops, long arg4, long arg5, long arg6);
extern long sys_semctl(long semid, long semnum, long cmd, long arg, long arg5, long arg6);
extern long sys_msgget(long key, long msgflg, long arg3, long arg4, long arg5, long arg6);
extern long sys_msgsnd(long msqid, long msgp, long msgsz, long msgflg, long arg5, long arg6);
extern long sys_msgrcv(long msqid, long msgp, long msgsz, long msgtyp, long msgflg, long arg6);
extern long sys_msgctl(long msqid, long cmd, long buf, long arg4, long arg5, long arg6);
extern long sys_shmget(long key, long size, long shmflg, long arg4, long arg5, long arg6);
extern long sys_shmat(long shmid, long shmaddr, long shmflg, long arg4, long arg5, long arg6);
extern long sys_shmdt(long shmaddr, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_shmctl(long shmid, long cmd, long buf, long arg4, long arg5, long arg6);
extern long sys_mq_open(long name, long oflag, long mode, long attr, long arg5, long arg6);
extern long sys_mq_unlink(long name, long arg2, long arg3, long arg4, long arg5, long arg6);
extern long sys_mq_timedsend(long mqdes, long msg_ptr, long msg_len, long msg_prio, long abs_timeout, long arg6);
extern long sys_mq_timedreceive(long mqdes, long msg_ptr, long msg_len, long msg_prio, long abs_timeout, long arg6);
extern long sys_mq_notify(long mqdes, long notification, long arg3, long arg4, long arg5, long arg6);
extern long sys_mq_getsetattr(long mqdes, long mqstat, long omqstat, long arg4, long arg5, long arg6);

/* External initialization functions */
extern void process_syscalls_init(void);
extern void fs_syscalls_init(void);
extern void mm_syscalls_init(void);
extern void time_syscalls_init(void);
extern void signal_syscalls_init(void);
extern void ipc_syscalls_init(void);
extern void net_syscalls_init(void);
extern void sysinfo_syscalls_init(void);
extern void security_syscalls_init(void);
extern void aio_syscalls_init(void);
extern void futex_syscalls_init(void);

/* Initialize the system call interface */
void syscall_init(void) {
    /* Initialize the system call table */
    for (u32 i = 0; i < MAX_SYSCALLS; i++) {
        syscall_table[i] = NULL;
    }

    /* Initialize subsystem-specific system calls */
    process_syscalls_init();
    fs_syscalls_init();
    mm_syscalls_init();
    time_syscalls_init();
    signal_syscalls_init();
    ipc_syscalls_init();
    net_syscalls_init();
    sysinfo_syscalls_init();
    security_syscalls_init();
    aio_syscalls_init();
    futex_syscalls_init();

    /* Additional system calls would be registered here */
    /* For Linux compatibility, we need to implement many more system calls */
}

/* Register a system call */
long syscall_register(u32 num, syscall_handler_t handler) {
    if (num >= MAX_SYSCALLS || handler == NULL) {
        return ERROR_INVAL;
    }

    syscall_table[num] = handler;
    return SUCCESS;
}

/* Unregister a system call */
long syscall_unregister(u32 num) {
    if (num >= MAX_SYSCALLS) {
        return ERROR_INVAL;
    }

    syscall_table[num] = NULL;
    return SUCCESS;
}

/* System call handler */
long syscall_handler(u32 num, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Check if the system call number is valid */
    if (num >= MAX_SYSCALLS) {
        return ERROR_INVAL;
    }

    /* Get the system call handler */
    syscall_handler_t handler = syscall_table[num];

    /* Check if the handler is valid */
    if (handler == NULL) {
        return -ENOSYS;
    }

    /* Call the handler */
    long result = handler(arg1, arg2, arg3, arg4, arg5, arg6);

    return result;
}
