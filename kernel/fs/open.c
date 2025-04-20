/*
 * open.c - Horizon kernel open system call implementation
 *
 * This file contains the implementation of the open system call and related
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/fs/vfs.h>
#include <horizon/fs/file.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <horizon/task.h>
#include <horizon/errno.h>
#include <horizon/fcntl.h>
#include <horizon/namei.h>
#include <horizon/sched.h>
#include <horizon/stat.h>
#include <horizon/uaccess.h>
#include <horizon/capability.h>
#include <horizon/security.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/**
 * do_sys_open - open a file descriptor
 * @pathname: pathname to open
 * @flags: open flags
 * @mode: file mode
 *
 * This function opens a file and returns a file descriptor.
 */
long do_sys_open(const char __user *pathname, int flags, umode_t mode)
{
    struct open_how how = {
        .flags = flags,
        .mode = mode,
    };

    return do_sys_openat2(AT_FDCWD, pathname, &how);
}

/**
 * do_sys_openat2 - open a file descriptor
 * @dfd: directory file descriptor
 * @filename: filename to open
 * @how: how to open the file
 *
 * This function opens a file and returns a file descriptor.
 */
long do_sys_openat2(int dfd, const char __user *filename, struct open_how *how)
{
    struct open_flags op;
    int fd = build_open_flags(how, &op);
    struct filename *tmp;

    if (fd)
        return fd;

    /* Get the filename */
    tmp = getname(filename);
    if (IS_ERR(tmp))
        return PTR_ERR(tmp);

    /* Get an unused file descriptor */
    fd = get_unused_fd_flags(how->flags);
    if (fd >= 0) {
        struct file *f = do_filp_open(dfd, tmp, &op);
        if (IS_ERR(f)) {
            put_unused_fd(fd);
            fd = PTR_ERR(f);
        } else {
            fsnotify_open(f);
            fd_install(fd, f);
        }
    }

    putname(tmp);
    return fd;
}

/**
 * build_open_flags - build open flags from open_how
 * @how: open_how structure
 * @op: open_flags structure to fill
 *
 * This function builds open flags from an open_how structure.
 * Returns 0 on success, or a negative error code.
 */
int build_open_flags(const struct open_how *how, struct open_flags *op)
{
    int flags = how->flags;
    int lookup_flags = 0;
    int acc_mode = ACC_MODE(flags);

    /* Check the validity of the flags */
    if ((flags & ~VALID_OPEN_FLAGS) || !acc_mode)
        return -EINVAL;

    /* O_PATH beats everything else */
    if (flags & O_PATH) {
        /* O_PATH only permits certain other flags */
        if (flags & ~O_PATH_FLAGS)
            return -EINVAL;
        acc_mode = 0;
    }

    /* O_TRUNC implies we need access checks for write permission */
    if (flags & O_TRUNC) {
        if (!(acc_mode & MAY_WRITE))
            return -EACCES;
    }

    /* Handle create flag */
    if (flags & O_CREAT) {
        op->mode = (how->mode & S_IALLUGO) | S_IFREG;
        if (!(flags & O_EXCL)) {
            lookup_flags |= LOOKUP_OPEN;
            if (acc_mode & MAY_WRITE)
                lookup_flags |= LOOKUP_OPEN_CREATE;
        }
    } else {
        op->mode = 0;
    }

    /* Handle directory flags */
    if (flags & O_DIRECTORY)
        lookup_flags |= LOOKUP_DIRECTORY;

    /* Handle follow symlinks flag */
    if (!(flags & O_NOFOLLOW))
        lookup_flags |= LOOKUP_FOLLOW;

    op->open_flag = flags;
    op->acc_mode = acc_mode;
    op->intent = flags & O_PATH ? LOOKUP_OPEN : LOOKUP_OPEN | LOOKUP_FOLLOW;
    if (flags & O_CREAT)
        op->intent |= LOOKUP_CREATE;
    if (flags & O_EXCL)
        op->intent |= LOOKUP_EXCL;
    if (flags & O_DIRECTORY)
        op->intent |= LOOKUP_DIRECTORY;
    op->lookup_flags = lookup_flags;

    return 0;
}

/**
 * vfs_create_file - create a file
 * @pathname: pathname to create
 * @mode: file mode
 * @path: path structure to fill
 *
 * This function creates a file and fills the path structure.
 * Returns 0 on success, or a negative error code.
 */
int vfs_create_file(const char *pathname, umode_t mode, struct path *path)
{
    /* Check parameters */
    if (pathname == NULL || path == NULL) {
        return -EINVAL;
    }

    /* Get the parent directory */
    char *parent_path = strdup(pathname);
    if (parent_path == NULL) {
        return -ENOMEM;
    }

    /* Find the last slash */
    char *last_slash = strrchr(parent_path, '/');
    if (last_slash == NULL) {
        kfree(parent_path);
        return -EINVAL;
    }

    /* Get the file name */
    char *name = last_slash + 1;
    if (*name == '\0') {
        kfree(parent_path);
        return -EINVAL;
    }

    /* Terminate the parent path */
    *last_slash = '\0';

    /* Find the parent directory */
    struct path parent;
    int error = vfs_kern_path(parent_path, LOOKUP_FOLLOW, &parent);

    kfree(parent_path);

    if (error) {
        return error;
    }

    /* Check if the parent is a directory */
    if (!S_ISDIR(parent.dentry->d_inode->i_mode)) {
        vfs_path_release(&parent);
        return -ENOTDIR;
    }

    /* Create a dentry for the new file */
    struct dentry *dentry = vfs_create_dentry(parent.dentry, name);
    if (dentry == NULL) {
        vfs_path_release(&parent);
        return -ENOMEM;
    }

    /* Create the file */
    error = vfs_create(parent.dentry->d_inode, dentry, mode, false);

    if (error) {
        /* Free the dentry */
        vfs_free_dentry(dentry);
        vfs_path_release(&parent);
        return error;
    }

    /* Set the path */
    path->mnt = parent.mnt;
    path->dentry = dentry;

    vfs_path_release(&parent);

    return 0;
}

/**
 * vfs_permission - check file permissions
 * @path: path to check
 * @mode: mode to check
 *
 * This function checks if the current task has permission to access
 * the file with the given mode.
 * Returns 0 on success, or a negative error code.
 */
int vfs_permission(const struct path *path, int mode)
{
    /* Check parameters */
    if (path == NULL) {
        return -EINVAL;
    }

    /* Get the inode */
    struct inode *inode = path->dentry->d_inode;
    if (inode == NULL) {
        return -ENOENT;
    }

    /* Get the current task */
    task_struct_t *task = task_current();
    if (task == NULL) {
        return -EINVAL;
    }

    /* Check if the user is root */
    if (task->euid == 0) {
        return 0;
    }

    /* Check the permissions */
    mode_t perm = inode->i_mode;

    /* Check if the user is the owner */
    if (task->euid == inode->i_uid) {
        perm >>= 6;
    }
    /* Check if the user is in the group */
    else if (task->egid == inode->i_gid) {
        perm >>= 3;
    }

    /* Check the permissions */
    if ((mode & O_RDONLY) && !(perm & S_IRUSR)) {
        return -EACCES;
    }

    if ((mode & O_WRONLY) && !(perm & S_IWUSR)) {
        return -EACCES;
    }

    if ((mode & O_RDWR) && (!(perm & S_IRUSR) || !(perm & S_IWUSR))) {
        return -EACCES;
    }

    return 0;
}

/**
 * vfs_create_dentry - create a dentry
 * @parent: parent dentry
 * @name: name of the dentry
 *
 * This function creates a new dentry under the given parent.
 * Returns the new dentry, or NULL on error.
 */
struct dentry *vfs_create_dentry(struct dentry *parent, const char *name)
{
    /* Check parameters */
    if (parent == NULL || name == NULL) {
        return NULL;
    }

    /* Allocate a dentry */
    struct dentry *dentry = kmalloc(sizeof(struct dentry), MEM_KERNEL | MEM_ZERO);
    if (dentry == NULL) {
        return NULL;
    }

    /* Initialize the dentry */
    dentry->d_name.name = strdup(name);
    if (dentry->d_name.name == NULL) {
        kfree(dentry);
        return NULL;
    }

    dentry->d_name.len = strlen(name);
    dentry->d_parent = parent;
    dentry->d_sb = parent->d_sb;
    dentry->d_inode = NULL;

    /* Initialize the lists */
    list_init(&dentry->d_child);
    list_init(&dentry->d_subdirs);

    /* Add the dentry to the parent */
    list_add(&dentry->d_child, &parent->d_subdirs);

    return dentry;
}

/**
 * vfs_free_dentry - free a dentry
 * @dentry: dentry to free
 *
 * This function frees a dentry and all associated resources.
 */
void vfs_free_dentry(struct dentry *dentry)
{
    /* Check parameters */
    if (dentry == NULL) {
        return;
    }

    /* Remove the dentry from the parent */
    list_del(&dentry->d_child);

    /* Free the name */
    if (dentry->d_name.name != NULL) {
        kfree(dentry->d_name.name);
    }

    /* Free the dentry */
    kfree(dentry);
}

/**
 * sys_open - open a file
 * @pathname: pathname to open
 * @flags: open flags
 * @mode: file mode
 * @unused1: unused parameter
 * @unused2: unused parameter
 * @unused3: unused parameter
 *
 * This function opens a file and returns a file descriptor.
 */
long sys_open(long pathname, long flags, long mode, long unused1, long unused2, long unused3)
{
    /* Force O_LARGEFILE on 32-bit systems */
    if (sizeof(long) == 4)
        flags |= O_LARGEFILE;

    /* Open the file */
    return do_sys_open((const char __user *)pathname, flags, mode);
}

/**
 * sys_openat - open a file relative to a directory file descriptor
 * @dirfd: directory file descriptor
 * @pathname: pathname to open
 * @flags: open flags
 * @mode: file mode
 * @unused1: unused parameter
 * @unused2: unused parameter
 *
 * This function opens a file relative to a directory file descriptor.
 */
long sys_openat(long dirfd, long pathname, long flags, long mode, long unused1, long unused2)
{
    /* Force O_LARGEFILE on 32-bit systems */
    if (sizeof(long) == 4)
        flags |= O_LARGEFILE;

    return do_sys_open(dirfd, (const char __user *)pathname, flags, mode);
}

/**
 * sys_creat - create a file
 * @pathname: pathname to create
 * @mode: file mode
 * @unused1: unused parameter
 * @unused2: unused parameter
 * @unused3: unused parameter
 * @unused4: unused parameter
 *
 * This function creates a file and returns a file descriptor.
 */
long sys_creat(long pathname, long mode, long unused1, long unused2, long unused3, long unused4)
{
    int flags = O_CREAT | O_WRONLY | O_TRUNC;

    /* Force O_LARGEFILE on 32-bit systems */
    if (sizeof(long) == 4)
        flags |= O_LARGEFILE;

    /* Create the file */
    return do_sys_open((const char __user *)pathname, flags, mode);
}

/**
 * vfs_d_path - get the path of a dentry
 * @path: path structure
 * @buf: buffer to store the path
 * @buflen: length of the buffer
 *
 * This function gets the path of a dentry and stores it in the buffer.
 * Returns the path length on success, or a negative error code.
 */
int vfs_d_path(const struct path *path, char *buf, int buflen)
{
    /* Check parameters */
    if (path == NULL || buf == NULL || buflen <= 0) {
        return -EINVAL;
    }

    /* Get the dentry */
    struct dentry *dentry = path->dentry;
    if (dentry == NULL) {
        return -EINVAL;
    }

    /* Check if this is the root dentry */
    if (dentry == root_dentry) {
        if (buflen < 2) {
            return -ERANGE;
        }

        buf[0] = '/';
        buf[1] = '\0';

        return 1;
    }

    /* Build the path */
    char *end = buf + buflen;
    char *p = end - 1;
    *p = '\0';

    /* Walk up the dentry tree */
    while (dentry != root_dentry) {
        /* Get the name */
        const char *name = dentry->d_name.name;
        int len = dentry->d_name.len;

        /* Check if we have enough space */
        p -= len;
        if (p < buf) {
            return -ERANGE;
        }

        /* Copy the name */
        memcpy(p, name, len);

        /* Add a slash */
        p--;
        if (p < buf) {
            return -ERANGE;
        }

        *p = '/';

        /* Move up to the parent */
        dentry = dentry->d_parent;
    }

    /* Check if we have enough space for the root */
    if (p > buf) {
        /* Move the path to the beginning of the buffer */
        int len = end - p - 1;
        memmove(buf, p, len + 1);
        return len;
    }

    return end - p - 1;
}
