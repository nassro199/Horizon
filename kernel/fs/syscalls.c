/**
 * syscalls.c - Horizon kernel file system system calls
 *
 * This file contains the implementation of the file system system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/fs/vfs.h>
#include <horizon/fs/file.h>
#include <horizon/task.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: stat */
long sys_stat(long pathname, long statbuf, long unused1, long unused2, long unused3, long unused4) {
    /* Get file status */
    return file_stat((const char *)pathname, (struct stat *)statbuf);
}

/* System call: lstat */
long sys_lstat(long pathname, long statbuf, long unused1, long unused2, long unused3, long unused4) {
    /* Get file status (don't follow links) */
    return file_lstat((const char *)pathname, (struct stat *)statbuf);
}

/* System call: fstat */
long sys_fstat(long fd, long statbuf, long unused1, long unused2, long unused3, long unused4) {
    /* Get the file */
    file_t *file = task_get_file(task_current(), fd);

    if (file == NULL) {
        return -1;
    }

    /* Get file status */
    return file_fstat(file, (struct stat *)statbuf);
}

/* System call: access */
long sys_access(long pathname, long mode, long unused1, long unused2, long unused3, long unused4) {
    /* Check file access permissions */
    return file_access((const char *)pathname, mode);
}

/* System call: mkdir */
long sys_mkdir(long pathname, long mode, long unused1, long unused2, long unused3, long unused4) {
    /* Create a directory */
    return file_mkdir((const char *)pathname, mode);
}

/* System call: rmdir */
long sys_rmdir(long pathname, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Remove a directory */
    return file_rmdir((const char *)pathname);
}

/* System call: creat */
long sys_creat(long pathname, long mode, long unused1, long unused2, long unused3, long unused4) {
    /* Create a file */
    return sys_open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode, 0, 0, 0);
}

/* System call: link */
long sys_link(long oldpath, long newpath, long unused1, long unused2, long unused3, long unused4) {
    /* Create a hard link */
    return file_link((const char *)oldpath, (const char *)newpath);
}

/* System call: unlink */
long sys_unlink(long pathname, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Remove a file */
    return file_unlink((const char *)pathname);
}

/* System call: symlink */
long sys_symlink(long target, long linkpath, long unused1, long unused2, long unused3, long unused4) {
    /* Create a symbolic link */
    return file_symlink((const char *)target, (const char *)linkpath);
}

/* System call: readlink */
long sys_readlink(long pathname, long buf, long bufsiz, long unused1, long unused2, long unused3) {
    /* Read the value of a symbolic link */
    return file_readlink((const char *)pathname, (char *)buf, bufsiz);
}

/* System call: chmod */
long sys_chmod(long pathname, long mode, long unused1, long unused2, long unused3, long unused4) {
    /* Change file permissions */
    return file_chmod((const char *)pathname, mode);
}

/* System call: fchmod */
long sys_fchmod(long fd, long mode, long unused1, long unused2, long unused3, long unused4) {
    /* Get the file */
    file_t *file = task_get_file(task_current(), fd);

    if (file == NULL) {
        return -1;
    }

    /* Change file permissions */
    struct path path;
    path.mnt = file->f_path.mnt;
    path.dentry = file->f_path.dentry;

    return vfs_chmod(&path, mode);
}

/* System call: chown */
long sys_chown(long pathname, long owner, long group, long unused1, long unused2, long unused3) {
    /* Change file owner and group */
    return file_chown((const char *)pathname, owner, group);
}

/* System call: fchown */
long sys_fchown(long fd, long owner, long group, long unused1, long unused2, long unused3) {
    /* Get the file */
    file_t *file = task_get_file(task_current(), fd);

    if (file == NULL) {
        return -1;
    }

    /* Change file owner and group */
    struct path path;
    path.mnt = file->f_path.mnt;
    path.dentry = file->f_path.dentry;

    return vfs_chown(&path, owner, group);
}

/* System call: lchown */
long sys_lchown(long pathname, long owner, long group, long unused1, long unused2, long unused3) {
    /* Change file owner and group (don't follow links) */
    struct path path;
    int error = vfs_kern_path((const char *)pathname, 0, &path);

    if (error) {
        return error;
    }

    error = vfs_chown(&path, owner, group);

    vfs_path_release(&path);

    return error;
}

/* System call: truncate */
long sys_truncate(long pathname, long length, long unused1, long unused2, long unused3, long unused4) {
    /* Truncate a file */
    struct path path;
    int error = vfs_kern_path((const char *)pathname, 0, &path);

    if (error) {
        return error;
    }

    error = vfs_truncate(&path, length);

    vfs_path_release(&path);

    return error;
}

/* System call: ftruncate */
long sys_ftruncate(long fd, long length, long unused1, long unused2, long unused3, long unused4) {
    /* Get the file */
    file_t *file = task_get_file(task_current(), fd);

    if (file == NULL) {
        return -1;
    }

    /* Truncate the file */
    return file_truncate(file, length);
}

/* System call: rename */
long sys_rename(long oldpath, long newpath, long unused1, long unused2, long unused3, long unused4) {
    /* Rename a file */
    return file_rename((const char *)oldpath, (const char *)newpath);
}

/* System call: chdir */
long sys_chdir(long pathname, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Change the current directory */
    return file_chdir((const char *)pathname);
}

/* System call: fchdir */
long sys_fchdir(long fd, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get the file */
    file_t *file = task_get_file(task_current(), fd);

    if (file == NULL) {
        return -1;
    }

    /* Change the current directory */
    return file_fchdir(file);
}

/* System call: getcwd */
long sys_getcwd(long buf, long size, long unused1, long unused2, long unused3, long unused4) {
    /* Get the current working directory */
    return (long)file_getcwd((char *)buf, size);
}

/* System call: dup */
long sys_dup(long fd, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get the file */
    file_t *file = task_get_file(task_current(), fd);

    if (file == NULL) {
        return -1;
    }

    /* Duplicate the file descriptor */
    return file_dup(file);
}

/* System call: dup2 */
long sys_dup2(long oldfd, long newfd, long unused1, long unused2, long unused3, long unused4) {
    /* Get the file */
    file_t *file = task_get_file(task_current(), oldfd);

    if (file == NULL) {
        return -1;
    }

    /* Duplicate the file descriptor */
    return file_dup2(file, newfd);
}

/* System call: fcntl */
long sys_fcntl(long fd, long cmd, long arg, long unused1, long unused2, long unused3) {
    /* Get the file */
    file_t *file = task_get_file(task_current(), fd);

    if (file == NULL) {
        return -1;
    }

    /* Perform the file control operation */
    return file_fcntl(file, cmd, arg);
}

/* System call: ioctl */
long sys_ioctl(long fd, long request, long arg, long unused1, long unused2, long unused3) {
    /* Get the file */
    file_t *file = task_get_file(task_current(), fd);

    if (file == NULL) {
        return -1;
    }

    /* Perform the I/O control operation */
    return file_ioctl(file, request, arg);
}

/* System call: pipe */
long sys_pipe(long pipefd, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Create a pipe */
    file_t *read_file, *write_file;
    int error = file_pipe(&read_file, &write_file);

    if (error) {
        return error;
    }

    /* Add the files to the task */
    int read_fd = task_add_file(task_current(), read_file);

    if (read_fd < 0) {
        file_close(read_file);
        file_close(write_file);
        return -1;
    }

    int write_fd = task_add_file(task_current(), write_file);

    if (write_fd < 0) {
        task_remove_file(task_current(), read_fd);
        file_close(write_file);
        return -1;
    }

    /* Set the pipe file descriptors */
    ((int *)pipefd)[0] = read_fd;
    ((int *)pipefd)[1] = write_fd;

    return 0;
}

/* System call: select */
long sys_select(long nfds, long readfds, long writefds, long exceptfds, long timeout, long unused1) {
    /* Wait for file descriptors to become ready */
    return file_select(nfds, (fd_set *)readfds, (fd_set *)writefds, (fd_set *)exceptfds, (struct timeval *)timeout);
}

/* System call: poll */
long sys_poll(long fds, long nfds, long timeout, long unused1, long unused2, long unused3) {
    /* Wait for events on file descriptors */
    return file_poll((struct pollfd *)fds, nfds, timeout);
}

/* System call: mount */
long sys_mount(long source, long target, long filesystemtype, long mountflags, long data, long unused1) {
    /* Mount a file system */
    return file_mount((const char *)source, (const char *)target, (const char *)filesystemtype, mountflags, (const void *)data);
}

/* System call: umount */
long sys_umount(long target, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Unmount a file system */
    return file_umount((const char *)target);
}

/* System call: umount2 */
long sys_umount2(long target, long flags, long unused1, long unused2, long unused3, long unused4) {
    /* Unmount a file system */
    return file_umount2((const char *)target, flags);
}

/* System call: statfs */
long sys_statfs(long pathname, long buf, long unused1, long unused2, long unused3, long unused4) {
    /* Get file system statistics */
    return file_statfs((const char *)pathname, (struct statfs *)buf);
}

/* System call: fstatfs */
long sys_fstatfs(long fd, long buf, long unused1, long unused2, long unused3, long unused4) {
    /* Get the file */
    file_t *file = task_get_file(task_current(), fd);

    if (file == NULL) {
        return -1;
    }

    /* Get file system statistics */
    return file_fstatfs(file, (struct statfs *)buf);
}

/* System call: sync */
long sys_sync(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Synchronize all file systems */
    return file_sync_all();
}

/* System call: fsync */
long sys_fsync(long fd, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get the file */
    file_t *file = task_get_file(task_current(), fd);

    if (file == NULL) {
        return -1;
    }

    /* Synchronize the file */
    return file_sync(file);
}

/* System call: fdatasync */
long sys_fdatasync(long fd, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get the file */
    file_t *file = task_get_file(task_current(), fd);

    if (file == NULL) {
        return -1;
    }

    /* Synchronize the file data */
    return file_datasync(file);
}

/* External initialization functions */
extern void fs_advio_syscalls_init(void);
extern void fs_xattr_syscalls_init(void);
extern void fs_notify_syscalls_init(void);

/* Register file system system calls */
void fs_syscalls_init(void) {
    /* Register file system system calls */
    syscall_register(SYS_STAT, sys_stat);
    syscall_register(SYS_LSTAT, sys_lstat);
    syscall_register(SYS_FSTAT, sys_fstat);
    syscall_register(SYS_ACCESS, sys_access);
    syscall_register(SYS_MKDIR, sys_mkdir);
    syscall_register(SYS_RMDIR, sys_rmdir);
    syscall_register(SYS_CREAT, sys_creat);
    syscall_register(SYS_LINK, sys_link);
    syscall_register(SYS_UNLINK, sys_unlink);
    syscall_register(SYS_SYMLINK, sys_symlink);
    syscall_register(SYS_READLINK, sys_readlink);
    syscall_register(SYS_CHMOD, sys_chmod);
    syscall_register(SYS_FCHMOD, sys_fchmod);
    syscall_register(SYS_CHOWN, sys_chown);
    syscall_register(SYS_FCHOWN, sys_fchown);
    syscall_register(SYS_LCHOWN, sys_lchown);
    syscall_register(SYS_TRUNCATE, sys_truncate);
    syscall_register(SYS_FTRUNCATE, sys_ftruncate);
    syscall_register(SYS_RENAME, sys_rename);
    syscall_register(SYS_CHDIR, sys_chdir);
    syscall_register(SYS_FCHDIR, sys_fchdir);
    syscall_register(SYS_GETCWD, sys_getcwd);
    syscall_register(SYS_DUP, sys_dup);
    syscall_register(SYS_DUP2, sys_dup2);
    syscall_register(SYS_FCNTL, sys_fcntl);
    syscall_register(SYS_IOCTL, sys_ioctl);
    syscall_register(SYS_PIPE, sys_pipe);
    syscall_register(SYS_SELECT, sys_select);
    syscall_register(SYS_POLL, sys_poll);
    syscall_register(SYS_MOUNT, sys_mount);
    syscall_register(SYS_UMOUNT, sys_umount);
    syscall_register(SYS_UMOUNT2, sys_umount2);
    syscall_register(SYS_STATFS, sys_statfs);
    syscall_register(SYS_FSTATFS, sys_fstatfs);
    syscall_register(SYS_SYNC, sys_sync);
    syscall_register(SYS_FSYNC, sys_fsync);
    syscall_register(SYS_FDATASYNC, sys_fdatasync);

    /* Initialize advanced I/O system calls */
    fs_advio_syscalls_init();

    /* Initialize extended attribute system calls */
    fs_xattr_syscalls_init();

    /* Initialize file notification system calls */
    fs_notify_syscalls_init();
}
