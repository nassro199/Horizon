/**
 * fs.c - Horizon kernel file system-related system calls
 *
 * This file contains the implementation of file system-related system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/fs.h>
#include <horizon/task.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Read system call */
long sys_read(long fd, long buffer, long size, long arg4, long arg5, long arg6) {
    /* Get the file from the file descriptor */
    struct file *file = task_current()->files->fd_array[fd];
    if (file == NULL) {
        return -EBADF;
    }

    /* Read from the file */
    return vfs_read(file, (char *)buffer, size, &file->f_pos);
}

/* Write system call */
long sys_write(long fd, long buffer, long size, long arg4, long arg5, long arg6) {
    /* Get the file from the file descriptor */
    struct file *file = task_current()->files->fd_array[fd];
    if (file == NULL) {
        return -EBADF;
    }

    /* Write to the file */
    return vfs_write(file, (const char *)buffer, size, &file->f_pos);
}

/* Open system call */
long sys_open(long pathname, long flags, long mode, long unused1, long unused2, long unused3) {
    /* Open a file */
    return vfs_open((const char *)pathname, flags, mode);
}

/* Openat system call */
long sys_openat(long dirfd, long pathname, long flags, long mode, long unused1, long unused2) {
    /* Open a file relative to a directory file descriptor */
    return vfs_openat(dirfd, (const char *)pathname, flags, mode);
}

/* Creat system call */
long sys_creat(long pathname, long mode, long unused1, long unused2, long unused3, long unused4) {
    /* Create a file */
    return vfs_open((const char *)pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

/* Close system call */
long sys_close(long fd, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Check if the file descriptor is valid */
    if (fd < 0 || fd >= task_current()->files->max_fds) {
        return -EBADF;
    }

    /* Get the file from the file descriptor */
    struct file *file = task_current()->files->fd_array[fd];
    if (file == NULL) {
        return -EBADF;
    }

    /* Close the file */
    int error = vfs_close(file);
    if (error) {
        return error;
    }

    /* Clear the file descriptor */
    task_current()->files->fd_array[fd] = NULL;

    return 0;
}

/* Seek system call */
long sys_lseek(long fd, long offset, long whence, long arg4, long arg5, long arg6) {
    /* Check if the file descriptor is valid */
    if (fd < 0 || fd >= task_current()->files->max_fds) {
        return -EBADF;
    }

    /* Get the file from the file descriptor */
    struct file *file = task_current()->files->fd_array[fd];
    if (file == NULL) {
        return -EBADF;
    }

    /* Seek in the file */
    return vfs_lseek(file, offset, whence);
}

/* Dup system call */
long sys_dup(long fd, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Check if the file descriptor is valid */
    if (fd < 0 || fd >= task_current()->files->max_fds) {
        return -EBADF;
    }

    /* Get the file from the file descriptor */
    struct file *file = task_current()->files->fd_array[fd];
    if (file == NULL) {
        return -EBADF;
    }

    /* Find a free file descriptor */
    int newfd = -1;
    for (int i = 0; i < task_current()->files->max_fds; i++) {
        if (task_current()->files->fd_array[i] == NULL) {
            newfd = i;
            break;
        }
    }

    /* Check if we found a free file descriptor */
    if (newfd == -1) {
        return -EMFILE;
    }

    /* Increment the file's reference count */
    file->f_count++;

    /* Set the new file descriptor */
    task_current()->files->fd_array[newfd] = file;

    return newfd;
}

/* Dup2 system call */
long sys_dup2(long oldfd, long newfd, long arg3, long arg4, long arg5, long arg6) {
    /* Check if the file descriptors are valid */
    if (oldfd < 0 || oldfd >= task_current()->files->max_fds ||
        newfd < 0 || newfd >= task_current()->files->max_fds) {
        return -EBADF;
    }

    /* Get the file from the old file descriptor */
    struct file *file = task_current()->files->fd_array[oldfd];
    if (file == NULL) {
        return -EBADF;
    }

    /* Check if the file descriptors are the same */
    if (oldfd == newfd) {
        return newfd;
    }

    /* Close the new file descriptor if it's open */
    if (task_current()->files->fd_array[newfd] != NULL) {
        vfs_close(task_current()->files->fd_array[newfd]);
    }

    /* Increment the file's reference count */
    file->f_count++;

    /* Set the new file descriptor */
    task_current()->files->fd_array[newfd] = file;

    return newfd;
}

/* Pipe system call */
long sys_pipe(long pipefd, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Create a pipe */
    return vfs_pipe((int *)pipefd);
}

/* Pipe2 system call */
long sys_pipe2(long pipefd, long flags, long arg3, long arg4, long arg5, long arg6) {
    /* Create a pipe with flags */
    return vfs_pipe2((int *)pipefd, flags);
}

/* Fcntl system call */
long sys_fcntl(long fd, long cmd, long arg, long arg4, long arg5, long arg6) {
    /* Check if the file descriptor is valid */
    if (fd < 0 || fd >= task_current()->files->max_fds) {
        return -EBADF;
    }

    /* Get the file from the file descriptor */
    struct file *file = task_current()->files->fd_array[fd];
    if (file == NULL) {
        return -EBADF;
    }

    /* Perform the fcntl operation */
    return vfs_fcntl(file, cmd, arg);
}

/* Ioctl system call */
long sys_ioctl(long fd, long cmd, long arg, long arg4, long arg5, long arg6) {
    /* Check if the file descriptor is valid */
    if (fd < 0 || fd >= task_current()->files->max_fds) {
        return -EBADF;
    }

    /* Get the file from the file descriptor */
    struct file *file = task_current()->files->fd_array[fd];
    if (file == NULL) {
        return -EBADF;
    }

    /* Perform the ioctl operation */
    return vfs_ioctl(file, cmd, arg);
}

/* Initialize file system-related system calls */
void fs_syscalls_init(void) {
    /* Register file system-related system calls */
    syscall_register(SYS_READ, sys_read);
    syscall_register(SYS_WRITE, sys_write);
    syscall_register(SYS_OPEN, sys_open);
    syscall_register(SYS_OPENAT, sys_openat);
    syscall_register(SYS_CREAT, sys_creat);
    syscall_register(SYS_CLOSE, sys_close);
    syscall_register(SYS_LSEEK, sys_lseek);
    syscall_register(SYS_DUP, sys_dup);
    syscall_register(SYS_DUP2, sys_dup2);
    syscall_register(SYS_PIPE, sys_pipe);
    syscall_register(SYS_FCNTL, sys_fcntl);
    syscall_register(SYS_IOCTL, sys_ioctl);
}
