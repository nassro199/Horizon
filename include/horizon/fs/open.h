/*
 * open.h - Horizon kernel open system call definitions
 *
 * This file contains definitions for the open system call.
 */

#ifndef _HORIZON_FS_OPEN_H
#define _HORIZON_FS_OPEN_H

#include <horizon/types.h>
#include <horizon/fs/vfs.h>

/* Open flags */
#define VALID_OPEN_FLAGS (O_RDONLY | O_WRONLY | O_RDWR | O_CREAT | O_EXCL | \
                         O_NOCTTY | O_TRUNC | O_APPEND | O_NONBLOCK | \
                         O_NDELAY | O_SYNC | O_ASYNC | O_DIRECT | \
                         O_LARGEFILE | O_DIRECTORY | O_NOFOLLOW | \
                         O_NOATIME | O_CLOEXEC | O_PATH | O_TMPFILE)

#define O_PATH_FLAGS (O_DIRECTORY | O_NOFOLLOW | O_PATH | O_CLOEXEC)

/* Access mode */
#define ACC_MODE(x) ((x) & O_ACCMODE)
#define MAY_READ    0x01
#define MAY_WRITE   0x02
#define MAY_EXEC    0x04
#define MAY_APPEND  0x08
#define MAY_ACCESS  0x10
#define MAY_OPEN    0x20
#define MAY_CHDIR   0x40

/* Lookup flags */
#define LOOKUP_FOLLOW        0x0001
#define LOOKUP_DIRECTORY     0x0002
#define LOOKUP_AUTOMOUNT     0x0004
#define LOOKUP_PARENT        0x0010
#define LOOKUP_REVAL         0x0020
#define LOOKUP_RCU           0x0040
#define LOOKUP_OPEN          0x0100
#define LOOKUP_CREATE        0x0200
#define LOOKUP_EXCL          0x0400
#define LOOKUP_RENAME_TARGET 0x0800
#define LOOKUP_OPEN_CREATE   0x1000

/* Open how structure */
struct open_how {
    u64 flags;
    u64 mode;
    u64 resolve;
};

/* Open flags structure */
struct open_flags {
    int open_flag;
    umode_t mode;
    int acc_mode;
    int intent;
    int lookup_flags;
};

/* Filename structure */
struct filename {
    const char *name;
    int refcnt;
};

/* Function prototypes */
long do_sys_open(const char __user *pathname, int flags, umode_t mode);
long do_sys_openat2(int dfd, const char __user *filename, struct open_how *how);
int build_open_flags(const struct open_how *how, struct open_flags *op);
struct file *do_filp_open(int dfd, struct filename *pathname, struct open_flags *op);
struct file *path_openat(struct nameidata *nd, struct open_flags *op, int flags);
struct file *do_open(struct nameidata *nd, struct open_flags *op);
int do_dentry_open(struct file *f, struct inode *inode, int (*open)(struct inode *, struct file *));
int get_unused_fd_flags(int flags);
int __alloc_fd(struct files_struct *files, unsigned start, unsigned end, int flags);
void fd_install(unsigned int fd, struct file *file);
int vfs_truncate(const struct path *path, loff_t length);
int do_truncate(struct idmap *idmap, struct dentry *dentry, loff_t length, unsigned int time_attrs, struct file *filp);
int vfs_create_file(const char *pathname, umode_t mode, struct path *path);
int vfs_permission(const struct path *path, int mode);
struct dentry *vfs_create_dentry(struct dentry *parent, const char *name);
void vfs_free_dentry(struct dentry *dentry);
int vfs_d_path(const struct path *path, char *buf, int buflen);

/* System calls */
long sys_open(long pathname, long flags, long mode, long unused1, long unused2, long unused3);
long sys_openat(long dirfd, long pathname, long flags, long mode, long unused1, long unused2);
long sys_creat(long pathname, long mode, long unused1, long unused2, long unused3, long unused4);

#endif /* _HORIZON_FS_OPEN_H */
