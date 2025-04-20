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

/* System call table */
syscall_handler_t syscall_table[MAX_SYSCALLS];

/* System call handlers */

/* Exit system call */
static long sys_exit(long status, long arg2, long arg3, long arg4, long arg5, long arg6) {
    sched_exit((int)status);
    return 0;
}

/* Read system call */
static long sys_read(long fd, long buffer, long size, long arg4, long arg5, long arg6) {
    /* Get the file from the file descriptor */
    struct file *file = current->files->fd_array[fd];
    if (file == NULL) {
        return ERROR_BADF;
    }

    /* Read from the file */
    return vfs_read(file, (char *)buffer, size, &file->f_pos);
}

/* Write system call */
static long sys_write(long fd, long buffer, long size, long arg4, long arg5, long arg6) {
    /* Get the file from the file descriptor */
    struct file *file = current->files->fd_array[fd];
    if (file == NULL) {
        return ERROR_BADF;
    }

    /* Write to the file */
    return vfs_write(file, (const char *)buffer, size, &file->f_pos);
}

/* Open system call */
static long sys_open(long path, long flags, long mode, long arg4, long arg5, long arg6) {
    /* Open the file */
    struct file *file;
    int error = vfs_open((const char *)path, &file, flags, mode);
    if (error) {
        return error;
    }

    /* Find a free file descriptor */
    int fd;
    for (fd = 0; fd < current->files->max_fds; fd++) {
        if (current->files->fd_array[fd] == NULL) {
            break;
        }
    }

    /* Check if we found a free file descriptor */
    if (fd >= current->files->max_fds) {
        vfs_close(file);
        return ERROR_MFILE;
    }

    /* Set the file descriptor */
    current->files->fd_array[fd] = file;

    return fd;
}

/* Close system call */
static long sys_close(long fd, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Check if the file descriptor is valid */
    if (fd < 0 || fd >= current->files->max_fds) {
        return ERROR_BADF;
    }

    /* Get the file from the file descriptor */
    struct file *file = current->files->fd_array[fd];
    if (file == NULL) {
        return ERROR_BADF;
    }

    /* Close the file */
    int error = vfs_close(file);
    if (error) {
        return error;
    }

    /* Clear the file descriptor */
    current->files->fd_array[fd] = NULL;

    return SUCCESS;
}

/* Fork system call */
static long sys_fork(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Create a new process */
    struct task_struct *child = sched_fork();
    if (child == NULL) {
        return ERROR_NOMEM;
    }

    /* Return the child's PID to the parent */
    return child->pid;
}

/* Exec system call */
static long sys_execve(long path, long argv, long envp, long arg4, long arg5, long arg6) {
    /* Execute a new program */
    return sched_exec((const char *)path, (char *const *)argv, (char *const *)envp);
}

/* Wait system call */
static long sys_wait4(long pid, long status, long options, long rusage, long arg5, long arg6) {
    /* Wait for a process to change state */
    return sched_wait(pid, (int *)status, options, (struct rusage *)rusage);
}

/* Get process ID system call */
static long sys_getpid(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Return the current process ID */
    return current->pid;
}

/* Get parent process ID system call */
static long sys_getppid(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Return the parent process ID */
    return current->parent->pid;
}

/* Memory map system call */
static long sys_mmap(long addr, long length, long prot, long flags, long fd, long offset) {
    /* Map a file into memory */
    struct file *file = NULL;
    if (fd >= 0 && fd < current->files->max_fds) {
        file = current->files->fd_array[fd];
    }

    /* Map the memory */
    void *mapped_addr;
    int error = vmm_mmap(current->mm, (void *)addr, length, prot, flags, file, offset, &mapped_addr);
    if (error) {
        return error;
    }

    return (long)mapped_addr;
}

/* Memory unmap system call */
static long sys_munmap(long addr, long length, long arg3, long arg4, long arg5, long arg6) {
    /* Unmap memory */
    return vmm_munmap(current->mm, (void *)addr, length);
}

/* Break system call */
static long sys_brk(long brk, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Change the program break */
    return vmm_brk(current->mm, brk);
}

/* Get time of day system call */
static long sys_gettimeofday(long tv, long tz, long arg3, long arg4, long arg5, long arg6) {
    /* Get the current time */
    struct timeval *tp = (struct timeval *)tv;
    struct timezone *tzp = (struct timezone *)tz;

    /* Set the time */
    if (tp != NULL) {
        tp->tv_sec = time_get_seconds();
        tp->tv_usec = time_get_microseconds();
    }

    /* Set the timezone */
    if (tzp != NULL) {
        tzp->tz_minuteswest = 0;
        tzp->tz_dsttime = 0;
    }

    return SUCCESS;
}

/* Nanosleep system call */
static long sys_nanosleep(long req, long rem, long arg3, long arg4, long arg5, long arg6) {
    /* Sleep for a specified time */
    struct timespec *tp = (struct timespec *)req;
    struct timespec *rmtp = (struct timespec *)rem;

    /* Check if the timespec is valid */
    if (tp == NULL) {
        return ERROR_INVAL;
    }

    /* Sleep */
    unsigned long timeout = tp->tv_sec * 1000 + tp->tv_nsec / 1000000;
    schedule_timeout_interruptible(timeout);

    /* Set the remaining time */
    if (rmtp != NULL) {
        rmtp->tv_sec = 0;
        rmtp->tv_nsec = 0;
    }

    return SUCCESS;
}

/* External initialization functions */
extern void fs_syscalls_init(void);
extern void mm_syscalls_init(void);
extern void time_syscalls_init(void);
extern void signal_syscalls_init(void);
extern void ipc_syscalls_init(void);
extern void net_syscalls_init(void);
extern void sysinfo_syscalls_init(void);
extern void security_syscalls_init(void);
extern void process_syscalls_init(void);
extern void aio_syscalls_init(void);
extern void futex_syscalls_init(void);

/* Initialize the system call interface */
void syscall_init(void) {
    /* Initialize the system call table */
    for (u32 i = 0; i < MAX_SYSCALLS; i++) {
        syscall_table[i] = NULL;
    }

    /* Register process-related system calls */
    syscall_register(SYS_EXIT, sys_exit);
    syscall_register(SYS_FORK, sys_fork);
    syscall_register(SYS_EXECVE, sys_execve);
    syscall_register(SYS_WAIT4, sys_wait4);
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
    syscall_register(SYS_CLONE, sys_clone);
    syscall_register(SYS_VFORK, sys_vfork);
    syscall_register(SYS_EXIT_GROUP, sys_exit_group);
    syscall_register(SYS_SET_TID_ADDRESS, sys_set_tid_address);
    syscall_register(SYS_GETTID, sys_gettid);
    syscall_register(SYS_SET_THREAD_AREA, sys_set_thread_area);
    syscall_register(SYS_GET_THREAD_AREA, sys_get_thread_area);
    syscall_register(SYS_WAITID, sys_waitid);

    /* Register file-related system calls */
    syscall_register(SYS_READ, sys_read);
    syscall_register(SYS_WRITE, sys_write);
    syscall_register(SYS_OPEN, sys_open);
    syscall_register(SYS_CLOSE, sys_close);

    /* Register memory-related system calls */
    syscall_register(SYS_BRK, sys_brk);
    syscall_register(SYS_MMAP, sys_mmap);
    syscall_register(SYS_MUNMAP, sys_munmap);

    /* Register time-related system calls */
    syscall_register(SYS_GETTIMEOFDAY, sys_gettimeofday);
    syscall_register(SYS_NANOSLEEP, sys_nanosleep);

    /* Initialize subsystem-specific system calls */
    fs_syscalls_init();
    mm_syscalls_init();
    time_syscalls_init();
    signal_syscalls_init();
    ipc_syscalls_init();
    net_syscalls_init();
    sysinfo_syscalls_init();
    security_syscalls_init();
    process_syscalls_init();
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
        return ERROR_NOSYS;
    }

    /* Call the handler */
    long result = handler(arg1, arg2, arg3, arg4, arg5, arg6);

    return result;
}
