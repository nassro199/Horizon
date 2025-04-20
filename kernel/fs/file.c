/**
 * file.c - Horizon kernel file implementation
 * 
 * This file contains the implementation of the file subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/fs/vfs.h>
#include <horizon/fs/file.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Open a file */
int file_open(const char *pathname, int flags, mode_t mode, file_t **file) {
    /* Check parameters */
    if (pathname == NULL || file == NULL) {
        return -1;
    }
    
    /* Allocate a file structure */
    *file = kmalloc(sizeof(file_t), MEM_KERNEL | MEM_ZERO);
    
    if (*file == NULL) {
        return -1;
    }
    
    /* Initialize the file structure */
    (*file)->f_flags = flags;
    (*file)->f_mode = mode;
    (*file)->f_pos = 0;
    
    /* Open the file */
    struct path path;
    int error = vfs_kern_path(pathname, 0, &path);
    
    if (error) {
        kfree(*file);
        *file = NULL;
        return error;
    }
    
    /* Set the file path */
    (*file)->f_path.mnt = path.mnt;
    (*file)->f_path.dentry = path.dentry;
    
    /* Get the inode */
    (*file)->f_inode = path.dentry->d_inode;
    
    /* Set the file operations */
    (*file)->f_op = (*file)->f_inode->i_fop;
    
    /* Call the open operation if available */
    if ((*file)->f_op && (*file)->f_op->open) {
        error = (*file)->f_op->open((*file)->f_inode, *file);
        
        if (error) {
            vfs_path_release(&path);
            kfree(*file);
            *file = NULL;
            return error;
        }
    }
    
    return 0;
}

/* Close a file */
int file_close(file_t *file) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* Call the release operation if available */
    if (file->f_op && file->f_op->release) {
        file->f_op->release(file->f_inode, file);
    }
    
    /* Release the path */
    vfs_path_release(&file->f_path);
    
    /* Free the file structure */
    kfree(file);
    
    return 0;
}

/* Read from a file */
ssize_t file_read(file_t *file, char *buf, size_t count) {
    /* Check parameters */
    if (file == NULL || buf == NULL) {
        return -1;
    }
    
    /* Check if the file is readable */
    if (!(file->f_mode & FMODE_READ)) {
        return -1;
    }
    
    /* Call the read operation if available */
    if (file->f_op && file->f_op->read) {
        return file->f_op->read(file, buf, count, &file->f_pos);
    }
    
    return -1;
}

/* Write to a file */
ssize_t file_write(file_t *file, const char *buf, size_t count) {
    /* Check parameters */
    if (file == NULL || buf == NULL) {
        return -1;
    }
    
    /* Check if the file is writable */
    if (!(file->f_mode & FMODE_WRITE)) {
        return -1;
    }
    
    /* Call the write operation if available */
    if (file->f_op && file->f_op->write) {
        return file->f_op->write(file, buf, count, &file->f_pos);
    }
    
    return -1;
}

/* Seek in a file */
off_t file_seek(file_t *file, off_t offset, int whence) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* Calculate the new position */
    loff_t pos;
    
    switch (whence) {
        case SEEK_SET:
            pos = offset;
            break;
        
        case SEEK_CUR:
            pos = file->f_pos + offset;
            break;
        
        case SEEK_END:
            pos = file->f_inode->i_size + offset;
            break;
        
        default:
            return -1;
    }
    
    /* Check if the position is valid */
    if (pos < 0) {
        return -1;
    }
    
    /* Call the llseek operation if available */
    if (file->f_op && file->f_op->llseek) {
        return file->f_op->llseek(file, offset, whence);
    }
    
    /* Set the position */
    file->f_pos = pos;
    
    return pos;
}

/* Get file status */
int file_stat(const char *pathname, struct stat *statbuf) {
    /* Check parameters */
    if (pathname == NULL || statbuf == NULL) {
        return -1;
    }
    
    /* Get the path */
    struct path path;
    int error = vfs_kern_path(pathname, 0, &path);
    
    if (error) {
        return error;
    }
    
    /* Get the inode */
    struct inode *inode = path.dentry->d_inode;
    
    /* Fill the stat structure */
    statbuf->st_dev = inode->i_sb->s_dev;
    statbuf->st_ino = inode->i_ino;
    statbuf->st_mode = inode->i_mode;
    statbuf->st_nlink = inode->i_nlink;
    statbuf->st_uid = inode->i_uid;
    statbuf->st_gid = inode->i_gid;
    statbuf->st_rdev = inode->i_rdev;
    statbuf->st_size = inode->i_size;
    statbuf->st_blksize = inode->i_sb->s_blocksize;
    statbuf->st_blocks = inode->i_blocks;
    statbuf->st_atime = inode->i_atime.tv_sec;
    statbuf->st_mtime = inode->i_mtime.tv_sec;
    statbuf->st_ctime = inode->i_ctime.tv_sec;
    
    /* Release the path */
    vfs_path_release(&path);
    
    return 0;
}

/* Get file status (don't follow links) */
int file_lstat(const char *pathname, struct stat *statbuf) {
    /* Check parameters */
    if (pathname == NULL || statbuf == NULL) {
        return -1;
    }
    
    /* Get the path */
    struct path path;
    int error = vfs_kern_path(pathname, LOOKUP_NOFOLLOW, &path);
    
    if (error) {
        return error;
    }
    
    /* Get the inode */
    struct inode *inode = path.dentry->d_inode;
    
    /* Fill the stat structure */
    statbuf->st_dev = inode->i_sb->s_dev;
    statbuf->st_ino = inode->i_ino;
    statbuf->st_mode = inode->i_mode;
    statbuf->st_nlink = inode->i_nlink;
    statbuf->st_uid = inode->i_uid;
    statbuf->st_gid = inode->i_gid;
    statbuf->st_rdev = inode->i_rdev;
    statbuf->st_size = inode->i_size;
    statbuf->st_blksize = inode->i_sb->s_blocksize;
    statbuf->st_blocks = inode->i_blocks;
    statbuf->st_atime = inode->i_atime.tv_sec;
    statbuf->st_mtime = inode->i_mtime.tv_sec;
    statbuf->st_ctime = inode->i_ctime.tv_sec;
    
    /* Release the path */
    vfs_path_release(&path);
    
    return 0;
}

/* Get file status */
int file_fstat(file_t *file, struct stat *statbuf) {
    /* Check parameters */
    if (file == NULL || statbuf == NULL) {
        return -1;
    }
    
    /* Get the inode */
    struct inode *inode = file->f_inode;
    
    /* Fill the stat structure */
    statbuf->st_dev = inode->i_sb->s_dev;
    statbuf->st_ino = inode->i_ino;
    statbuf->st_mode = inode->i_mode;
    statbuf->st_nlink = inode->i_nlink;
    statbuf->st_uid = inode->i_uid;
    statbuf->st_gid = inode->i_gid;
    statbuf->st_rdev = inode->i_rdev;
    statbuf->st_size = inode->i_size;
    statbuf->st_blksize = inode->i_sb->s_blocksize;
    statbuf->st_blocks = inode->i_blocks;
    statbuf->st_atime = inode->i_atime.tv_sec;
    statbuf->st_mtime = inode->i_mtime.tv_sec;
    statbuf->st_ctime = inode->i_ctime.tv_sec;
    
    return 0;
}

/* Check file access permissions */
int file_access(const char *pathname, int mode) {
    /* Check parameters */
    if (pathname == NULL) {
        return -1;
    }
    
    /* Get the path */
    struct path path;
    int error = vfs_kern_path(pathname, 0, &path);
    
    if (error) {
        return error;
    }
    
    /* Check the permissions */
    error = vfs_permission(&path, mode);
    
    /* Release the path */
    vfs_path_release(&path);
    
    return error;
}

/* Create a directory */
int file_mkdir(const char *pathname, mode_t mode) {
    /* Check parameters */
    if (pathname == NULL) {
        return -1;
    }
    
    /* Get the parent directory */
    struct path path;
    char *name;
    int error = vfs_kern_path_parent(pathname, &path, &name);
    
    if (error) {
        return error;
    }
    
    /* Create the directory */
    error = vfs_mkdir(path.dentry, name, mode);
    
    /* Release the path */
    vfs_path_release(&path);
    
    /* Free the name */
    kfree(name);
    
    return error;
}

/* Remove a directory */
int file_rmdir(const char *pathname) {
    /* Check parameters */
    if (pathname == NULL) {
        return -1;
    }
    
    /* Get the parent directory */
    struct path path;
    char *name;
    int error = vfs_kern_path_parent(pathname, &path, &name);
    
    if (error) {
        return error;
    }
    
    /* Remove the directory */
    error = vfs_rmdir(path.dentry, name);
    
    /* Release the path */
    vfs_path_release(&path);
    
    /* Free the name */
    kfree(name);
    
    return error;
}

/* Create a hard link */
int file_link(const char *oldpath, const char *newpath) {
    /* Check parameters */
    if (oldpath == NULL || newpath == NULL) {
        return -1;
    }
    
    /* Get the old path */
    struct path old_path;
    int error = vfs_kern_path(oldpath, 0, &old_path);
    
    if (error) {
        return error;
    }
    
    /* Get the new parent directory */
    struct path new_path;
    char *name;
    error = vfs_kern_path_parent(newpath, &new_path, &name);
    
    if (error) {
        vfs_path_release(&old_path);
        return error;
    }
    
    /* Create the link */
    error = vfs_link(old_path.dentry, new_path.dentry, name);
    
    /* Release the paths */
    vfs_path_release(&old_path);
    vfs_path_release(&new_path);
    
    /* Free the name */
    kfree(name);
    
    return error;
}

/* Remove a file */
int file_unlink(const char *pathname) {
    /* Check parameters */
    if (pathname == NULL) {
        return -1;
    }
    
    /* Get the parent directory */
    struct path path;
    char *name;
    int error = vfs_kern_path_parent(pathname, &path, &name);
    
    if (error) {
        return error;
    }
    
    /* Remove the file */
    error = vfs_unlink(path.dentry, name);
    
    /* Release the path */
    vfs_path_release(&path);
    
    /* Free the name */
    kfree(name);
    
    return error;
}

/* Create a symbolic link */
int file_symlink(const char *target, const char *linkpath) {
    /* Check parameters */
    if (target == NULL || linkpath == NULL) {
        return -1;
    }
    
    /* Get the parent directory */
    struct path path;
    char *name;
    int error = vfs_kern_path_parent(linkpath, &path, &name);
    
    if (error) {
        return error;
    }
    
    /* Create the symbolic link */
    error = vfs_symlink(path.dentry, name, target);
    
    /* Release the path */
    vfs_path_release(&path);
    
    /* Free the name */
    kfree(name);
    
    return error;
}

/* Read the value of a symbolic link */
int file_readlink(const char *pathname, char *buf, size_t bufsiz) {
    /* Check parameters */
    if (pathname == NULL || buf == NULL) {
        return -1;
    }
    
    /* Get the path */
    struct path path;
    int error = vfs_kern_path(pathname, LOOKUP_NOFOLLOW, &path);
    
    if (error) {
        return error;
    }
    
    /* Check if the path is a symbolic link */
    if (!S_ISLNK(path.dentry->d_inode->i_mode)) {
        vfs_path_release(&path);
        return -1;
    }
    
    /* Read the symbolic link */
    error = vfs_readlink(path.dentry, buf, bufsiz);
    
    /* Release the path */
    vfs_path_release(&path);
    
    return error;
}

/* Change file permissions */
int file_chmod(const char *pathname, mode_t mode) {
    /* Check parameters */
    if (pathname == NULL) {
        return -1;
    }
    
    /* Get the path */
    struct path path;
    int error = vfs_kern_path(pathname, 0, &path);
    
    if (error) {
        return error;
    }
    
    /* Change the permissions */
    error = vfs_chmod(&path, mode);
    
    /* Release the path */
    vfs_path_release(&path);
    
    return error;
}

/* Change file owner and group */
int file_chown(const char *pathname, uid_t owner, gid_t group) {
    /* Check parameters */
    if (pathname == NULL) {
        return -1;
    }
    
    /* Get the path */
    struct path path;
    int error = vfs_kern_path(pathname, 0, &path);
    
    if (error) {
        return error;
    }
    
    /* Change the owner and group */
    error = vfs_chown(&path, owner, group);
    
    /* Release the path */
    vfs_path_release(&path);
    
    return error;
}

/* Truncate a file */
int file_truncate(file_t *file, off_t length) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* Check if the file is writable */
    if (!(file->f_mode & FMODE_WRITE)) {
        return -1;
    }
    
    /* Call the truncate operation if available */
    if (file->f_op && file->f_op->truncate) {
        return file->f_op->truncate(file->f_inode, length);
    }
    
    return -1;
}

/* Rename a file */
int file_rename(const char *oldpath, const char *newpath) {
    /* Check parameters */
    if (oldpath == NULL || newpath == NULL) {
        return -1;
    }
    
    /* Get the old parent directory */
    struct path old_path;
    char *old_name;
    int error = vfs_kern_path_parent(oldpath, &old_path, &old_name);
    
    if (error) {
        return error;
    }
    
    /* Get the new parent directory */
    struct path new_path;
    char *new_name;
    error = vfs_kern_path_parent(newpath, &new_path, &new_name);
    
    if (error) {
        vfs_path_release(&old_path);
        kfree(old_name);
        return error;
    }
    
    /* Rename the file */
    error = vfs_rename(old_path.dentry, old_name, new_path.dentry, new_name);
    
    /* Release the paths */
    vfs_path_release(&old_path);
    vfs_path_release(&new_path);
    
    /* Free the names */
    kfree(old_name);
    kfree(new_name);
    
    return error;
}

/* Change the current directory */
int file_chdir(const char *pathname) {
    /* Check parameters */
    if (pathname == NULL) {
        return -1;
    }
    
    /* Get the path */
    struct path path;
    int error = vfs_kern_path(pathname, 0, &path);
    
    if (error) {
        return error;
    }
    
    /* Check if the path is a directory */
    if (!S_ISDIR(path.dentry->d_inode->i_mode)) {
        vfs_path_release(&path);
        return -1;
    }
    
    /* Set the current directory */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        vfs_path_release(&path);
        return -1;
    }
    
    /* Release the old path */
    if (task->fs && task->fs->pwd.dentry) {
        vfs_path_release(&task->fs->pwd);
    }
    
    /* Set the new path */
    if (task->fs) {
        task->fs->pwd = path;
    }
    
    return 0;
}

/* Change the current directory */
int file_fchdir(file_t *file) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* Check if the file is a directory */
    if (!S_ISDIR(file->f_inode->i_mode)) {
        return -1;
    }
    
    /* Set the current directory */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Release the old path */
    if (task->fs && task->fs->pwd.dentry) {
        vfs_path_release(&task->fs->pwd);
    }
    
    /* Set the new path */
    if (task->fs) {
        task->fs->pwd.mnt = file->f_path.mnt;
        task->fs->pwd.dentry = dget(file->f_path.dentry);
    }
    
    return 0;
}

/* Get the current working directory */
char *file_getcwd(char *buf, size_t size) {
    /* Check parameters */
    if (buf == NULL) {
        return NULL;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return NULL;
    }
    
    /* Check if the task has a file system */
    if (task->fs == NULL || task->fs->pwd.dentry == NULL) {
        return NULL;
    }
    
    /* Get the path */
    char *path = vfs_dentry_path(task->fs->pwd.dentry, buf, size);
    
    return path;
}

/* Duplicate a file descriptor */
int file_dup(file_t *file) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Add the file to the task */
    return task_add_file(task, file);
}

/* Duplicate a file descriptor */
int file_dup2(file_t *file, int newfd) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -1;
    }
    
    /* Check if the new file descriptor is valid */
    if (newfd < 0 || newfd >= task->files->max_fds) {
        return -1;
    }
    
    /* Close the old file descriptor if it's open */
    if (task->files->fd_array[newfd] != NULL) {
        file_close(task->files->fd_array[newfd]);
    }
    
    /* Set the new file descriptor */
    task->files->fd_array[newfd] = file;
    
    return newfd;
}

/* Perform a file control operation */
int file_fcntl(file_t *file, int cmd, long arg) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* Perform the operation */
    switch (cmd) {
        case F_DUPFD:
            /* Duplicate the file descriptor */
            return file_dup(file);
        
        case F_GETFD:
            /* Get the close-on-exec flag */
            return 0;
        
        case F_SETFD:
            /* Set the close-on-exec flag */
            return 0;
        
        case F_GETFL:
            /* Get the file status flags */
            return file->f_flags;
        
        case F_SETFL:
            /* Set the file status flags */
            file->f_flags = arg;
            return 0;
        
        default:
            return -1;
    }
}

/* Perform an I/O control operation */
int file_ioctl(file_t *file, unsigned int cmd, unsigned long arg) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* Call the ioctl operation if available */
    if (file->f_op && file->f_op->ioctl) {
        return file->f_op->ioctl(file->f_inode, file, cmd, arg);
    }
    
    return -1;
}

/* Create a pipe */
int file_pipe(file_t **read_file, file_t **write_file) {
    /* Check parameters */
    if (read_file == NULL || write_file == NULL) {
        return -1;
    }
    
    /* Create the pipe */
    return vfs_pipe(read_file, write_file);
}

/* Synchronize a file */
int file_sync(file_t *file) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* Call the fsync operation if available */
    if (file->f_op && file->f_op->fsync) {
        return file->f_op->fsync(file, 0);
    }
    
    return 0;
}

/* Synchronize a file's data */
int file_datasync(file_t *file) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* Call the fsync operation if available */
    if (file->f_op && file->f_op->fsync) {
        return file->f_op->fsync(file, 1);
    }
    
    return 0;
}

/* Synchronize all file systems */
int file_sync_all(void) {
    /* Synchronize all file systems */
    return vfs_sync_all();
}

/* Mount a file system */
int file_mount(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data) {
    /* Check parameters */
    if (target == NULL || filesystemtype == NULL) {
        return -1;
    }
    
    /* Mount the file system */
    return vfs_mount(source, target, filesystemtype, mountflags, data);
}

/* Unmount a file system */
int file_umount(const char *target) {
    /* Check parameters */
    if (target == NULL) {
        return -1;
    }
    
    /* Unmount the file system */
    return vfs_umount(target);
}

/* Unmount a file system */
int file_umount2(const char *target, int flags) {
    /* Check parameters */
    if (target == NULL) {
        return -1;
    }
    
    /* Unmount the file system */
    return vfs_umount2(target, flags);
}

/* Get file system statistics */
int file_statfs(const char *path, struct statfs *buf) {
    /* Check parameters */
    if (path == NULL || buf == NULL) {
        return -1;
    }
    
    /* Get the path */
    struct path p;
    int error = vfs_kern_path(path, 0, &p);
    
    if (error) {
        return error;
    }
    
    /* Get the file system statistics */
    error = vfs_statfs(&p, buf);
    
    /* Release the path */
    vfs_path_release(&p);
    
    return error;
}

/* Get file system statistics */
int file_fstatfs(file_t *file, struct statfs *buf) {
    /* Check parameters */
    if (file == NULL || buf == NULL) {
        return -1;
    }
    
    /* Get the file system statistics */
    struct path path;
    path.mnt = file->f_path.mnt;
    path.dentry = file->f_path.dentry;
    
    return vfs_statfs(&path, buf);
}

/* Wait for file descriptors to become ready */
int file_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
    /* This would be implemented with actual select */
    return 0;
}

/* Wait for events on file descriptors */
int file_poll(struct pollfd *fds, nfds_t nfds, int timeout) {
    /* This would be implemented with actual poll */
    return 0;
}

/* Map a file into memory */
int file_mmap(file_t *file, void *addr, size_t length, int prot, int flags, off_t offset, void **mapped_addr) {
    /* Check parameters */
    if (mapped_addr == NULL) {
        return -1;
    }
    
    /* Map the file */
    return vmm_mmap(task_current()->mm, addr, length, prot, flags, file, offset, mapped_addr);
}

/* Unmap a file from memory */
int file_munmap(void *addr, size_t length) {
    /* Unmap the file */
    return vmm_munmap(task_current()->mm, addr, length);
}

/* Create a special file */
int file_mknod(const char *pathname, mode_t mode, dev_t dev) {
    /* Check parameters */
    if (pathname == NULL) {
        return -1;
    }
    
    /* Get the parent directory */
    struct path path;
    char *name;
    int error = vfs_kern_path_parent(pathname, &path, &name);
    
    if (error) {
        return error;
    }
    
    /* Create the special file */
    error = vfs_mknod(path.dentry, name, mode, dev);
    
    /* Release the path */
    vfs_path_release(&path);
    
    /* Free the name */
    kfree(name);
    
    return error;
}
