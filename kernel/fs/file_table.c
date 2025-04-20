/**
 * file_table.c - Horizon kernel file table implementation
 *
 * This file contains the implementation of the file table subsystem.
 * The implementation is compatible with Linux.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/fs/vfs.h>
#include <horizon/fs/file.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <horizon/task.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Maximum number of file descriptors */
#define NR_OPEN_DEFAULT 64
#define NR_OPEN_MAX     1024

/* File table */
typedef struct files_struct {
    int count;                   /* Reference count */
    struct fdtable *fdt;         /* File descriptor table */
    struct fdtable fdtab;        /* File descriptor table */
    spinlock_t file_lock;        /* File lock */
    int next_fd;                 /* Next free file descriptor */
    struct file *fd_array[NR_OPEN_DEFAULT]; /* File array */
    unsigned long close_on_exec_init[1]; /* Close on exec flags */
    unsigned long open_fds_init[1]; /* Open file descriptors */
} files_struct_t;

/* File descriptor table */
typedef struct fdtable {
    unsigned int max_fds;        /* Maximum number of file descriptors */
    struct file **fd;            /* File array */
    unsigned long *close_on_exec; /* Close on exec flags */
    unsigned long *open_fds;     /* Open file descriptors */
} fdtable_t;

/* Initialize the file table */
void file_table_init(void) {
    /* Initialize the file table */
    /* This would be implemented with actual file table initialization */
}

/* Allocate a file table */
struct files_struct *files_alloc(void) {
    /* Allocate a file table */
    struct files_struct *files = kmalloc(sizeof(struct files_struct), MEM_KERNEL | MEM_ZERO);

    if (files == NULL) {
        return NULL;
    }

    /* Initialize the file table */
    files->count = 1;
    files->fdt = &files->fdtab;
    files->fdtab.max_fds = NR_OPEN_DEFAULT;
    files->fdtab.fd = files->fd_array;
    files->fdtab.close_on_exec = files->close_on_exec_init;
    files->fdtab.open_fds = files->open_fds_init;
    files->next_fd = 0;

    /* Initialize the locks */
    /* This would be implemented with actual lock initialization */

    return files;
}

/* Free a file table */
void files_free(struct files_struct *files) {
    if (files == NULL) {
        return;
    }

    /* Decrement the reference count */
    files->count--;

    /* Check if the file table is still in use */
    if (files->count > 0) {
        return;
    }

    /* Close all open files */
    for (int i = 0; i < files->fdt->max_fds; i++) {
        if (files->fdt->fd[i] != NULL) {
            vfs_close(files->fdt->fd[i]);
        }
    }

    /* Free the file table */
    if (files->fdt != &files->fdtab) {
        kfree(files->fdt->fd);
        kfree(files->fdt->close_on_exec);
        kfree(files->fdt->open_fds);
        kfree(files->fdt);
    }

    kfree(files);
}

/* Clone a file table */
struct files_struct *files_clone(struct files_struct *old_files) {
    if (old_files == NULL) {
        return NULL;
    }

    /* Allocate a new file table */
    struct files_struct *new_files = files_alloc();

    if (new_files == NULL) {
        return NULL;
    }

    /* Copy the file table */
    for (int i = 0; i < old_files->fdt->max_fds; i++) {
        if (old_files->fdt->fd[i] != NULL) {
            /* Increment the file reference count */
            /* This would be implemented with actual reference counting */

            /* Copy the file */
            new_files->fdt->fd[i] = old_files->fdt->fd[i];
        }
    }

    /* Copy the close on exec flags */
    for (int i = 0; i < (old_files->fdt->max_fds + 31) / 32; i++) {
        new_files->fdt->close_on_exec[i] = old_files->fdt->close_on_exec[i];
    }

    /* Copy the open file descriptors */
    for (int i = 0; i < (old_files->fdt->max_fds + 31) / 32; i++) {
        new_files->fdt->open_fds[i] = old_files->fdt->open_fds[i];
    }

    /* Copy the next file descriptor */
    new_files->next_fd = old_files->next_fd;

    return new_files;
}

/* Expand the file table */
int expand_files(struct files_struct *files, int nr) {
    if (files == NULL || nr <= files->fdt->max_fds) {
        return -1;
    }

    /* Round up to the next power of 2 */
    int nfds = 1;
    while (nfds < nr) {
        nfds <<= 1;
    }

    /* Check if the number of file descriptors is too large */
    if (nfds > NR_OPEN_MAX) {
        nfds = NR_OPEN_MAX;
    }

    /* Allocate a new file descriptor table */
    struct fdtable *new_fdt = kmalloc(sizeof(struct fdtable), MEM_KERNEL | MEM_ZERO);

    if (new_fdt == NULL) {
        return -1;
    }

    /* Allocate a new file array */
    struct file **new_fd = kmalloc(nfds * sizeof(struct file *), MEM_KERNEL | MEM_ZERO);

    if (new_fd == NULL) {
        kfree(new_fdt);
        return -1;
    }

    /* Allocate new close on exec flags */
    unsigned long *new_close_on_exec = kmalloc((nfds + 31) / 32 * sizeof(unsigned long), MEM_KERNEL | MEM_ZERO);

    if (new_close_on_exec == NULL) {
        kfree(new_fd);
        kfree(new_fdt);
        return -1;
    }

    /* Allocate new open file descriptors */
    unsigned long *new_open_fds = kmalloc((nfds + 31) / 32 * sizeof(unsigned long), MEM_KERNEL | MEM_ZERO);

    if (new_open_fds == NULL) {
        kfree(new_close_on_exec);
        kfree(new_fd);
        kfree(new_fdt);
        return -1;
    }

    /* Copy the file array */
    for (int i = 0; i < files->fdt->max_fds; i++) {
        new_fd[i] = files->fdt->fd[i];
    }

    /* Copy the close on exec flags */
    for (int i = 0; i < (files->fdt->max_fds + 31) / 32; i++) {
        new_close_on_exec[i] = files->fdt->close_on_exec[i];
    }

    /* Copy the open file descriptors */
    for (int i = 0; i < (files->fdt->max_fds + 31) / 32; i++) {
        new_open_fds[i] = files->fdt->open_fds[i];
    }

    /* Initialize the new file descriptor table */
    new_fdt->max_fds = nfds;
    new_fdt->fd = new_fd;
    new_fdt->close_on_exec = new_close_on_exec;
    new_fdt->open_fds = new_open_fds;

    /* Free the old file descriptor table */
    if (files->fdt != &files->fdtab) {
        kfree(files->fdt->fd);
        kfree(files->fdt->close_on_exec);
        kfree(files->fdt->open_fds);
        kfree(files->fdt);
    }

    /* Set the new file descriptor table */
    files->fdt = new_fdt;

    return 0;
}

/* Allocate a file descriptor */
int alloc_fd(struct files_struct *files, int start, int flags) {
    if (files == NULL) {
        return -1;
    }

    /* Find a free file descriptor */
    int fd = start;

    while (fd < files->fdt->max_fds) {
        if (files->fdt->fd[fd] == NULL) {
            /* Set the file descriptor as open */
            files->fdt->open_fds[fd / 32] |= 1UL << (fd & 31);

            /* Set the close on exec flag */
            if (flags & O_CLOEXEC) {
                files->fdt->close_on_exec[fd / 32] |= 1UL << (fd & 31);
            } else {
                files->fdt->close_on_exec[fd / 32] &= ~(1UL << (fd & 31));
            }

            /* Update the next file descriptor */
            if (fd >= files->next_fd) {
                files->next_fd = fd + 1;
            }

            return fd;
        }

        fd++;
    }

    /* Expand the file table */
    if (expand_files(files, fd + 1) < 0) {
        return -1;
    }

    /* Set the file descriptor as open */
    files->fdt->open_fds[fd / 32] |= 1UL << (fd & 31);

    /* Set the close on exec flag */
    if (flags & O_CLOEXEC) {
        files->fdt->close_on_exec[fd / 32] |= 1UL << (fd & 31);
    } else {
        files->fdt->close_on_exec[fd / 32] &= ~(1UL << (fd & 31));
    }

    /* Update the next file descriptor */
    if (fd >= files->next_fd) {
        files->next_fd = fd + 1;
    }

    return fd;
}

/* Free a file descriptor */
void free_fd(struct files_struct *files, int fd) {
    if (files == NULL || fd < 0 || fd >= files->fdt->max_fds) {
        return;
    }

    /* Close the file */
    if (files->fdt->fd[fd] != NULL) {
        vfs_close(files->fdt->fd[fd]);
        files->fdt->fd[fd] = NULL;
    }

    /* Clear the file descriptor as open */
    files->fdt->open_fds[fd / 32] &= ~(1UL << (fd & 31));

    /* Clear the close on exec flag */
    files->fdt->close_on_exec[fd / 32] &= ~(1UL << (fd & 31));

    /* Update the next file descriptor */
    if (fd < files->next_fd) {
        files->next_fd = fd;
    }
}

/* Get a file from a file descriptor */
struct file *fget(int fd) {
    /* Get the current task */
    struct task_struct *task = task_current();

    if (task == NULL || task->files == NULL) {
        return NULL;
    }

    /* Check if the file descriptor is valid */
    if (fd < 0 || fd >= task->files->fdt->max_fds) {
        return NULL;
    }

    /* Get the file */
    struct file *file = task->files->fdt->fd[fd];

    if (file == NULL) {
        return NULL;
    }

    /* Increment the file reference count */
    /* This would be implemented with actual reference counting */

    return file;
}

/* Put a file */
void fput(struct file *file) {
    if (file == NULL) {
        return;
    }

    /* Decrement the file reference count */
    /* This would be implemented with actual reference counting */
}

/* System call: open - now implemented in kernel/fs/open.c */
extern int do_sys_open(const char *pathname, int flags, mode_t mode);

/* System call: close */
int sys_close(int fd) {
    /* Get the current task */
    struct task_struct *task = task_current();

    if (task == NULL || task->files == NULL) {
        return -1;
    }

    /* Check if the file descriptor is valid */
    if (fd < 0 || fd >= task->files->fdt->max_fds) {
        return -1;
    }

    /* Free the file descriptor */
    free_fd(task->files, fd);

    return 0;
}

/* System call: read */
ssize_t sys_read(int fd, void *buf, size_t count) {
    /* Get the file */
    struct file *file = fget(fd);

    if (file == NULL) {
        return -1;
    }

    /* Read from the file */
    ssize_t ret = vfs_read(file, buf, count, &file->f_pos);

    /* Put the file */
    fput(file);

    return ret;
}

/* System call: write */
ssize_t sys_write(int fd, const void *buf, size_t count) {
    /* Get the file */
    struct file *file = fget(fd);

    if (file == NULL) {
        return -1;
    }

    /* Write to the file */
    ssize_t ret = vfs_write(file, buf, count, &file->f_pos);

    /* Put the file */
    fput(file);

    return ret;
}

/* System call: lseek */
off_t sys_lseek(int fd, off_t offset, int whence) {
    /* Get the file */
    struct file *file = fget(fd);

    if (file == NULL) {
        return -1;
    }

    /* Seek the file */
    loff_t pos = file->f_pos;

    switch (whence) {
        case SEEK_SET:
            pos = offset;
            break;

        case SEEK_CUR:
            pos += offset;
            break;

        case SEEK_END:
            pos = file->f_inode->i_size + offset;
            break;

        default:
            fput(file);
            return -1;
    }

    /* Check if the position is valid */
    if (pos < 0) {
        fput(file);
        return -1;
    }

    /* Set the file position */
    file->f_pos = pos;

    /* Put the file */
    fput(file);

    return pos;
}

/* System call: dup */
int sys_dup(int oldfd) {
    /* Get the current task */
    struct task_struct *task = task_current();

    if (task == NULL || task->files == NULL) {
        return -1;
    }

    /* Check if the file descriptor is valid */
    if (oldfd < 0 || oldfd >= task->files->fdt->max_fds) {
        return -1;
    }

    /* Get the file */
    struct file *file = task->files->fdt->fd[oldfd];

    if (file == NULL) {
        return -1;
    }

    /* Allocate a new file descriptor */
    int newfd = alloc_fd(task->files, 0, 0);

    if (newfd < 0) {
        return -1;
    }

    /* Increment the file reference count */
    /* This would be implemented with actual reference counting */

    /* Set the new file descriptor */
    task->files->fdt->fd[newfd] = file;

    return newfd;
}

/* System call: dup2 */
int sys_dup2(int oldfd, int newfd) {
    /* Get the current task */
    struct task_struct *task = task_current();

    if (task == NULL || task->files == NULL) {
        return -1;
    }

    /* Check if the file descriptors are valid */
    if (oldfd < 0 || oldfd >= task->files->fdt->max_fds || newfd < 0) {
        return -1;
    }

    /* Check if the old file descriptor is open */
    struct file *file = task->files->fdt->fd[oldfd];

    if (file == NULL) {
        return -1;
    }

    /* Check if the new file descriptor is the same as the old one */
    if (oldfd == newfd) {
        return newfd;
    }

    /* Check if the new file descriptor is too large */
    if (newfd >= task->files->fdt->max_fds) {
        /* Expand the file table */
        if (expand_files(task->files, newfd + 1) < 0) {
            return -1;
        }
    }

    /* Close the new file descriptor if it is open */
    if (task->files->fdt->fd[newfd] != NULL) {
        free_fd(task->files, newfd);
    }

    /* Increment the file reference count */
    /* This would be implemented with actual reference counting */

    /* Set the new file descriptor */
    task->files->fdt->fd[newfd] = file;

    /* Set the file descriptor as open */
    task->files->fdt->open_fds[newfd / 32] |= 1UL << (newfd & 31);

    /* Clear the close on exec flag */
    task->files->fdt->close_on_exec[newfd / 32] &= ~(1UL << (newfd & 31));

    /* Update the next file descriptor */
    if (newfd >= task->files->next_fd) {
        task->files->next_fd = newfd + 1;
    }

    return newfd;
}

/* System call: fcntl */
int sys_fcntl(int fd, int cmd, unsigned long arg) {
    /* Get the current task */
    struct task_struct *task = task_current();

    if (task == NULL || task->files == NULL) {
        return -1;
    }

    /* Check if the file descriptor is valid */
    if (fd < 0 || fd >= task->files->fdt->max_fds) {
        return -1;
    }

    /* Get the file */
    struct file *file = task->files->fdt->fd[fd];

    if (file == NULL) {
        return -1;
    }

    /* Handle the command */
    switch (cmd) {
        case F_DUPFD:
            /* Duplicate the file descriptor */
            return sys_dup2(fd, arg);

        case F_GETFD:
            /* Get the close on exec flag */
            return (task->files->fdt->close_on_exec[fd / 32] & (1UL << (fd & 31))) ? FD_CLOEXEC : 0;

        case F_SETFD:
            /* Set the close on exec flag */
            if (arg & FD_CLOEXEC) {
                task->files->fdt->close_on_exec[fd / 32] |= 1UL << (fd & 31);
            } else {
                task->files->fdt->close_on_exec[fd / 32] &= ~(1UL << (fd & 31));
            }
            return 0;

        case F_GETFL:
            /* Get the file flags */
            return file->f_flags;

        case F_SETFL:
            /* Set the file flags */
            file->f_flags = (file->f_flags & ~O_ACCMODE) | (arg & O_ACCMODE);
            return 0;

        default:
            /* Handle other commands */
            /* This would be implemented with actual command handling */
            return -1;
    }
}
