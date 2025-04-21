/**
 * fs.h - File system definitions
 *
 * This file contains definitions for the virtual file system.
 */

#ifndef _HORIZON_FS_H
#define _HORIZON_FS_H

#include <horizon/types.h>
#include <horizon/error.h>
#include <horizon/list.h>

/* Forward declarations */
struct vm_area_struct;
struct stat;

/* File types */
typedef enum {
    FILE_TYPE_REGULAR,
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_CHAR_DEVICE,
    FILE_TYPE_BLOCK_DEVICE,
    FILE_TYPE_PIPE,
    FILE_TYPE_SYMLINK,
    FILE_TYPE_SOCKET
} file_type_t;

/* File permissions */
#define FILE_PERM_READ        0x04    /* Read permission */
#define FILE_PERM_WRITE       0x02    /* Write permission */
#define FILE_PERM_EXEC        0x01    /* Execute permission */
#define FILE_PERM_USER_READ   0x0400  /* User read permission */
#define FILE_PERM_USER_WRITE  0x0200  /* User write permission */
#define FILE_PERM_USER_EXEC   0x0100  /* User execute permission */
#define FILE_PERM_GROUP_READ  0x0040  /* Group read permission */
#define FILE_PERM_GROUP_WRITE 0x0020  /* Group write permission */
#define FILE_PERM_GROUP_EXEC  0x0010  /* Group execute permission */
#define FILE_PERM_OTHER_READ  0x0004  /* Other read permission */
#define FILE_PERM_OTHER_WRITE 0x0002  /* Other write permission */
#define FILE_PERM_OTHER_EXEC  0x0001  /* Other execute permission */
#define FILE_PERM_SETUID      0x0800  /* Set user ID on execution */
#define FILE_PERM_SETGID      0x0400  /* Set group ID on execution */
#define FILE_PERM_STICKY      0x0200  /* Sticky bit */

/* File open flags */
#define FILE_OPEN_READ        0x01    /* Open for reading */
#define FILE_OPEN_WRITE       0x02    /* Open for writing */
#define FILE_OPEN_CREATE      0x04    /* Create if it doesn't exist */
#define FILE_OPEN_APPEND      0x08    /* Append to the file */
#define FILE_OPEN_TRUNC       0x10    /* Truncate the file */
#define FILE_OPEN_EXCL        0x20    /* Fail if file exists and FILE_OPEN_CREATE is set */
#define FILE_OPEN_SYNC        0x40    /* Synchronous writes */
#define FILE_OPEN_NONBLOCK    0x80    /* Non-blocking mode */
#define FILE_OPEN_DIRECTORY   0x100   /* Open directory */
#define FILE_OPEN_NOFOLLOW    0x200   /* Don't follow symlinks */
#define FILE_OPEN_CLOEXEC     0x400   /* Close on exec */

/* Inode structure */
typedef struct inode {
    u32 inode_num;            /* Inode number */
    file_type_t type;         /* File type */
    u32 permissions;          /* File permissions */
    u32 uid;                  /* User ID */
    u32 gid;                  /* Group ID */
    u64 size;                 /* File size */
    u64 blocks;               /* Number of blocks allocated */
    u64 atime;                /* Access time */
    u64 mtime;                /* Modification time */
    u64 ctime;                /* Creation time */
    u32 links;                /* Number of hard links */
    void *fs_data;            /* File system specific data */
    struct inode_operations *i_ops; /* Inode operations */
} inode_t;

/* Dentry structure */
typedef struct dentry {
    char name[256];           /* Entry name */
    struct inode *inode;      /* Inode */
    struct dentry *parent;    /* Parent directory */
    struct list_head children; /* Child entries */
    struct list_head siblings; /* Sibling entries */
    void *fs_data;            /* File system specific data */
    struct dentry_operations *d_ops; /* Dentry operations */
} dentry_t;

/* File structure */
typedef struct file {
    char name[256];           /* File name */
    file_type_t type;         /* File type */
    u32 permissions;          /* File permissions */
    u64 size;                 /* File size */
    u64 position;             /* Current position */
    u32 flags;                /* Open flags */
    struct inode *inode;      /* Inode */
    struct dentry *dentry;    /* Directory entry */
    void *fs_data;            /* File system specific data */
    struct file_operations *f_ops; /* File operations */
} file_t;

/* Inode operations */
typedef struct inode_operations {
    struct inode *(*lookup)(struct inode *dir, const char *name);
    error_t (*create)(struct inode *dir, const char *name, u32 mode, struct inode **inode);
    error_t (*link)(struct inode *inode, struct inode *dir, const char *name);
    error_t (*unlink)(struct inode *dir, const char *name);
    error_t (*symlink)(struct inode *dir, const char *name, const char *target);
    error_t (*mkdir)(struct inode *dir, const char *name, u32 mode);
    error_t (*rmdir)(struct inode *dir, const char *name);
    error_t (*rename)(struct inode *old_dir, const char *old_name, struct inode *new_dir, const char *new_name);
    error_t (*readlink)(struct inode *inode, char *buffer, size_t size);
    error_t (*follow_link)(struct inode *inode, struct inode **target);
    error_t (*truncate)(struct inode *inode, u64 size);
    error_t (*permission)(struct inode *inode, int mask);
    error_t (*setattr)(struct inode *inode, struct iattr *attr);
    error_t (*getattr)(struct inode *inode, struct iattr *attr);
    struct super_block *(*get_super)(struct inode *inode);
} inode_operations_t;

/* Dentry operations */
typedef struct dentry_operations {
    int (*compare)(const char *name1, const char *name2);
    int (*hash)(const char *name);
    error_t (*delete)(struct dentry *dentry);
    error_t (*release)(struct dentry *dentry);
    error_t (*iput)(struct dentry *dentry, struct inode *inode);
} dentry_operations_t;

/* File operations */
typedef struct file_operations {
    ssize_t (*read)(file_t *file, void *buffer, size_t size);
    ssize_t (*write)(file_t *file, const void *buffer, size_t size);
    error_t (*open)(file_t *file, u32 flags);
    error_t (*close)(file_t *file);
    error_t (*seek)(file_t *file, u64 offset, int whence);
    loff_t (*llseek)(file_t *file, loff_t offset, int whence);
    error_t (*flush)(file_t *file);
    error_t (*fsync)(file_t *file);
    int (*ioctl)(file_t *file, unsigned int cmd, unsigned long arg);
    int (*mmap)(file_t *file, struct vm_area_struct *vma);
    int (*poll)(file_t *file, struct poll_table *wait);
    ssize_t (*readdir)(file_t *file, void *dirent, size_t count);
    error_t (*lock)(file_t *file, int cmd, struct file_lock *lock);
    error_t (*flock)(file_t *file, int cmd, struct file_lock *lock);
} file_operations_t;

/* File attribute structure */
typedef struct iattr {
    u32 ia_valid;              /* Validation mask */
    u32 ia_mode;               /* File mode */
    u32 ia_uid;                /* User ID */
    u32 ia_gid;                /* Group ID */
    u64 ia_size;               /* File size */
    u64 ia_atime;              /* Access time */
    u64 ia_mtime;              /* Modification time */
    u64 ia_ctime;              /* Creation time */
} iattr_t;

/* File lock structure */
typedef struct file_lock {
    u32 l_type;                /* Type of lock */
    u32 l_whence;              /* How to interpret l_start */
    u64 l_start;               /* Starting offset */
    u64 l_len;                 /* Length of locked area */
    u32 l_pid;                 /* Process ID of the process holding the lock */
} file_lock_t;

/* Poll table structure */
typedef struct poll_table {
    void *table;               /* Poll table */
} poll_table_t;

/* Mount flags */
#define MOUNT_READ_ONLY   0x01    /* Read-only mount */
#define MOUNT_NO_EXEC     0x02    /* Do not allow execution of binaries */
#define MOUNT_NO_DEV      0x04    /* Do not interpret character or block special devices */
#define MOUNT_NO_SUID     0x08    /* Do not allow set-user-identifier or set-group-identifier bits to take effect */

/* Directory entry structure */
typedef struct dirent {
    u32 inode;                 /* Inode number */
    u32 type;                  /* File type */
    char name[256];            /* File name */
} dirent_t;

/* Superblock structure */
typedef struct super_block {
    u32 magic;                 /* Magic number */
    u32 block_size;            /* Block size */
    u64 total_blocks;          /* Total number of blocks */
    u64 free_blocks;           /* Number of free blocks */
    u64 total_inodes;          /* Total number of inodes */
    u64 free_inodes;           /* Number of free inodes */
    u32 flags;                 /* Mount flags */
    void *fs_data;             /* File system specific data */
    struct super_operations *s_ops; /* Superblock operations */
} super_block_t;

/* Superblock operations */
typedef struct super_operations {
    struct inode *(*alloc_inode)(struct super_block *sb);
    void (*destroy_inode)(struct super_block *sb, struct inode *inode);
    error_t (*write_inode)(struct super_block *sb, struct inode *inode);
    error_t (*read_inode)(struct super_block *sb, struct inode *inode);
    error_t (*put_super)(struct super_block *sb);
    error_t (*write_super)(struct super_block *sb);
    error_t (*statfs)(struct super_block *sb, struct statfs *buf);
    error_t (*remount_fs)(struct super_block *sb, int *flags);
} super_operations_t;

/* File system type structure */
typedef struct file_system_type {
    char name[32];             /* File system name */
    u32 flags;                 /* File system flags */
    struct super_block *(*get_super)(const char *dev, u32 flags);
    int (*mount)(const char *dev, const char *dir, u32 flags);
    int (*unmount)(const char *dir);
    struct file_system_type *next;
} file_system_type_t;

/* File statistics structure */
typedef struct stat {
    u32 st_dev;                /* Device ID containing file */
    u32 st_ino;                /* Inode number */
    u32 st_mode;               /* File mode */
    u32 st_nlink;              /* Number of hard links */
    u32 st_uid;                /* User ID of owner */
    u32 st_gid;                /* Group ID of owner */
    u32 st_rdev;               /* Device ID (if special file) */
    u64 st_size;               /* Total size in bytes */
    u64 st_blksize;            /* Block size for filesystem I/O */
    u64 st_blocks;             /* Number of 512B blocks allocated */
    u64 st_atime;              /* Time of last access */
    u64 st_mtime;              /* Time of last modification */
    u64 st_ctime;              /* Time of last status change */
} stat_t;

/* File system statistics structure */
typedef struct statfs {
    u32 type;                  /* File system type */
    u32 block_size;            /* Optimal transfer block size */
    u64 blocks;                /* Total data blocks in file system */
    u64 blocks_free;           /* Free blocks in file system */
    u64 blocks_avail;          /* Free blocks available to unprivileged user */
    u64 files;                 /* Total file nodes in file system */
    u64 files_free;            /* Free file nodes in file system */
    u32 namelen;               /* Maximum length of filenames */
} statfs_t;

/* File system functions */
void fs_init(void);
int fs_register(const char *name, int (*mount)(const char *dev, const char *dir, u32 flags), int (*unmount)(const char *dir));
int fs_mount(const char *dev, const char *dir, const char *fs_name, u32 flags);
int fs_mount_super(const char *dir, struct super_block *super);
int fs_unmount_super(const char *dir);
struct super_block *fs_get_super(const char *dir);
struct inode *fs_lookup(const char *path);
int fs_unmount(const char *dir);
file_t *fs_open(const char *path, u32 flags);
error_t fs_close(file_t *file);
ssize_t fs_read(file_t *file, void *buffer, size_t size);
ssize_t fs_write(file_t *file, const void *buffer, size_t size);
error_t fs_seek(file_t *file, u64 offset, int whence);
error_t fs_flush(file_t *file);
error_t fs_fsync(file_t *file);
int fs_ioctl(file_t *file, unsigned int cmd, unsigned long arg);
error_t fs_truncate(const char *path, u64 size);
error_t fs_ftruncate(file_t *file, u64 size);
error_t fs_stat(const char *path, struct stat *buf);
error_t fs_fstat(file_t *file, struct stat *buf);
error_t fs_mkdir(const char *path, u32 mode);
error_t fs_rmdir(const char *path);
error_t fs_rename(const char *oldpath, const char *newpath);
error_t fs_link(const char *oldpath, const char *newpath);
error_t fs_unlink(const char *path);
error_t fs_symlink(const char *target, const char *linkpath);
error_t fs_readlink(const char *path, char *buf, size_t size);
error_t fs_chmod(const char *path, u32 mode);
error_t fs_fchmod(file_t *file, u32 mode);
error_t fs_chown(const char *path, u32 uid, u32 gid);
error_t fs_fchown(file_t *file, u32 uid, u32 gid);
error_t fs_statfs(const char *path, struct statfs *buf);
error_t fs_fstatfs(file_t *file, struct statfs *buf);
file_t *fs_opendir(const char *path);
error_t fs_closedir(file_t *dir);
error_t fs_readdir(file_t *dir, struct dirent *dirent);
error_t fs_rewinddir(file_t *dir);
error_t fs_seekdir(file_t *dir, u64 offset);
u64 fs_telldir(file_t *dir);

#endif /* _HORIZON_FS_H */
