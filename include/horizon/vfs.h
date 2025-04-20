/**
 * vfs.h - Horizon kernel Virtual File System definitions
 * 
 * This file contains definitions for the Virtual File System (VFS) layer.
 * The definitions are compatible with Linux.
 */

#ifndef _KERNEL_VFS_H
#define _KERNEL_VFS_H

#include <horizon/types.h>
#include <horizon/list.h>

/* File system types */
#define FSTYPE_NONE      0
#define FSTYPE_EXT2      1
#define FSTYPE_FAT       2
#define FSTYPE_ISO9660   3
#define FSTYPE_PROC      4
#define FSTYPE_DEVFS     5
#define FSTYPE_SYSFS     6
#define FSTYPE_TMPFS     7
#define FSTYPE_RAMFS     8
#define FSTYPE_ROOTFS    9
#define FSTYPE_MAX       10

/* File types */
#define S_IFMT   0170000  /* Mask for file type */
#define S_IFSOCK 0140000  /* Socket */
#define S_IFLNK  0120000  /* Symbolic link */
#define S_IFREG  0100000  /* Regular file */
#define S_IFBLK  0060000  /* Block device */
#define S_IFDIR  0040000  /* Directory */
#define S_IFCHR  0020000  /* Character device */
#define S_IFIFO  0010000  /* FIFO */

/* File permissions */
#define S_ISUID  0004000  /* Set user ID on execution */
#define S_ISGID  0002000  /* Set group ID on execution */
#define S_ISVTX  0001000  /* Sticky bit */
#define S_IRWXU  0000700  /* User (file owner) has read, write, and execute permission */
#define S_IRUSR  0000400  /* User has read permission */
#define S_IWUSR  0000200  /* User has write permission */
#define S_IXUSR  0000100  /* User has execute permission */
#define S_IRWXG  0000070  /* Group has read, write, and execute permission */
#define S_IRGRP  0000040  /* Group has read permission */
#define S_IWGRP  0000020  /* Group has write permission */
#define S_IXGRP  0000010  /* Group has execute permission */
#define S_IRWXO  0000007  /* Others have read, write, and execute permission */
#define S_IROTH  0000004  /* Others have read permission */
#define S_IWOTH  0000002  /* Others have write permission */
#define S_IXOTH  0000001  /* Others have execute permission */

/* File type macros */
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)   /* Is a symbolic link */
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)   /* Is a regular file */
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)   /* Is a directory */
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)   /* Is a character device */
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)   /* Is a block device */
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)   /* Is a FIFO */
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)  /* Is a socket */

/* File open flags */
#define O_RDONLY    0x0000  /* Open for reading only */
#define O_WRONLY    0x0001  /* Open for writing only */
#define O_RDWR      0x0002  /* Open for reading and writing */
#define O_ACCMODE   0x0003  /* Mask for file access modes */
#define O_CREAT     0x0100  /* Create if nonexistent */
#define O_EXCL      0x0200  /* Error if already exists */
#define O_NOCTTY    0x0400  /* Don't assign controlling terminal */
#define O_TRUNC     0x1000  /* Truncate to zero length */
#define O_APPEND    0x2000  /* Append to the file */
#define O_NONBLOCK  0x4000  /* Non-blocking I/O */
#define O_SYNC      0x101000 /* Synchronous I/O */
#define O_ASYNC     0x2000  /* Signal-driven I/O */
#define O_DIRECT    0x4000  /* Direct I/O */
#define O_LARGEFILE 0x8000  /* Allow files larger than 2GB */
#define O_DIRECTORY 0x10000 /* Must be a directory */
#define O_NOFOLLOW  0x20000 /* Don't follow symbolic links */
#define O_NOATIME   0x40000 /* Don't update access time */
#define O_CLOEXEC   0x80000 /* Close on exec */
#define O_PATH      0x100000 /* Path only */
#define O_TMPFILE   0x200000 /* Temporary file */

/* File seek whence values */
#define SEEK_SET    0  /* Seek from beginning of file */
#define SEEK_CUR    1  /* Seek from current position */
#define SEEK_END    2  /* Seek from end of file */

/* Forward declarations */
struct super_block;
struct inode;
struct dentry;
struct file;
struct vfsmount;
struct file_system_type;

/* File system operations */
typedef struct super_operations {
    struct inode *(*alloc_inode)(struct super_block *sb);
    void (*destroy_inode)(struct inode *inode);
    void (*dirty_inode)(struct inode *inode);
    int (*write_inode)(struct inode *inode, int wait);
    void (*drop_inode)(struct inode *inode);
    void (*delete_inode)(struct inode *inode);
    void (*put_super)(struct super_block *sb);
    int (*sync_fs)(struct super_block *sb, int wait);
    int (*freeze_fs)(struct super_block *sb);
    int (*unfreeze_fs)(struct super_block *sb);
    int (*statfs)(struct super_block *sb, void *buf);
    int (*remount_fs)(struct super_block *sb, int *flags, char *data);
    void (*clear_inode)(struct inode *inode);
    void (*umount_begin)(struct super_block *sb);
} super_operations_t;

/* Inode operations */
typedef struct inode_operations {
    int (*create)(struct inode *dir, struct dentry *dentry, u32 mode, struct nameidata *nd);
    struct dentry *(*lookup)(struct inode *dir, struct dentry *dentry, struct nameidata *nd);
    int (*link)(struct dentry *old_dentry, struct inode *dir, struct dentry *dentry);
    int (*unlink)(struct inode *dir, struct dentry *dentry);
    int (*symlink)(struct inode *dir, struct dentry *dentry, const char *symname);
    int (*mkdir)(struct inode *dir, struct dentry *dentry, u32 mode);
    int (*rmdir)(struct inode *dir, struct dentry *dentry);
    int (*mknod)(struct inode *dir, struct dentry *dentry, u32 mode, dev_t dev);
    int (*rename)(struct inode *old_dir, struct dentry *old_dentry, struct inode *new_dir, struct dentry *new_dentry);
    int (*readlink)(struct dentry *dentry, char *buffer, int buflen);
    void *(*follow_link)(struct dentry *dentry, struct nameidata *nd);
    void (*put_link)(struct dentry *dentry, struct nameidata *nd, void *cookie);
    int (*permission)(struct inode *inode, int mask);
    int (*setattr)(struct dentry *dentry, struct iattr *attr);
    int (*getattr)(struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat);
} inode_operations_t;

/* File operations */
typedef struct file_operations {
    int (*open)(struct inode *inode, struct file *file);
    int (*release)(struct inode *inode, struct file *file);
    ssize_t (*read)(struct file *file, char *buf, size_t count, loff_t *pos);
    ssize_t (*write)(struct file *file, const char *buf, size_t count, loff_t *pos);
    int (*flush)(struct file *file);
    int (*fsync)(struct file *file, struct dentry *dentry, int datasync);
    loff_t (*llseek)(struct file *file, loff_t offset, int whence);
    int (*readdir)(struct file *file, void *dirent, int (*filldir)(void *, const char *, int, loff_t, u64, unsigned));
    unsigned int (*poll)(struct file *file, struct poll_table_struct *wait);
    int (*ioctl)(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
    int (*mmap)(struct file *file, struct vm_area_struct *vma);
    int (*fasync)(int fd, struct file *file, int on);
    int (*lock)(struct file *file, int cmd, struct file_lock *lock);
    ssize_t (*sendpage)(struct file *file, struct page *page, int offset, size_t size, loff_t *pos, int more);
    unsigned long (*get_unmapped_area)(struct file *file, unsigned long addr, unsigned long len, unsigned long pgoff, unsigned long flags);
    int (*check_flags)(int flags);
    int (*flock)(struct file *file, int cmd, struct file_lock *lock);
    ssize_t (*splice_write)(struct pipe_inode_info *pipe, struct file *out, loff_t *ppos, size_t len, unsigned int flags);
    ssize_t (*splice_read)(struct file *in, loff_t *ppos, struct pipe_inode_info *pipe, size_t len, unsigned int flags);
    int (*setlease)(struct file *file, long arg, struct file_lock **lease);
} file_operations_t;

/* Directory entry operations */
typedef struct dentry_operations {
    int (*d_revalidate)(struct dentry *dentry, struct nameidata *nd);
    int (*d_hash)(const struct dentry *dentry, const struct inode *inode, struct qstr *name);
    int (*d_compare)(const struct dentry *parent, const struct inode *pinode, const struct dentry *dentry, const struct inode *inode, unsigned int len, const char *str, const struct qstr *name);
    int (*d_delete)(struct dentry *dentry);
    void (*d_release)(struct dentry *dentry);
    void (*d_iput)(struct dentry *dentry, struct inode *inode);
    char *(*d_dname)(struct dentry *dentry, char *buffer, int buflen);
} dentry_operations_t;

/* File system type operations */
typedef struct file_system_type {
    const char *name;
    int fs_flags;
    struct super_block *(*get_sb)(struct file_system_type *fs_type, int flags, const char *dev_name, void *data);
    void (*kill_sb)(struct super_block *sb);
    struct module *owner;
    struct file_system_type *next;
} file_system_type_t;

/* Super block structure */
typedef struct super_block {
    dev_t s_dev;                     /* Device identifier */
    unsigned long s_blocksize;       /* Block size */
    unsigned char s_blocksize_bits;  /* Block size bits */
    unsigned char s_dirt;            /* Dirty flag */
    unsigned long long s_maxbytes;   /* Max file size */
    struct file_system_type *s_type; /* File system type */
    super_operations_t *s_op;        /* Super block operations */
    struct dentry *s_root;           /* Root directory entry */
    struct list_head s_inodes;       /* Inode list */
    struct list_head s_dirty;        /* Dirty inode list */
    struct list_head s_io;           /* IO inode list */
    struct list_head s_more_io;      /* More IO inode list */
    struct list_head s_files;        /* File list */
    struct list_head s_dentry_lru;   /* Dentry LRU list */
    int s_nr_dentry_unused;          /* Number of unused dentries */
    struct block_device *s_bdev;     /* Block device */
    struct mtd_info *s_mtd;          /* MTD info */
    struct list_head s_instances;    /* Instances */
    struct quota_info s_dquot;       /* Quota info */
    int s_frozen;                    /* Frozen state */
    wait_queue_head_t s_wait_unfrozen; /* Wait queue for unfreezing */
    char s_id[32];                   /* Identifier */
    void *s_fs_info;                 /* File system specific info */
    fmode_t s_mode;                  /* Mount mode */
    struct mutex s_vfs_rename_mutex; /* Rename mutex */
    u32 s_time_gran;                 /* Granularity of timestamps */
    char *s_subtype;                 /* Subtype name */
    char *s_options;                 /* Mount options */
} super_block_t;

/* Inode structure */
typedef struct inode {
    u32 i_mode;                      /* Access mode */
    uid_t i_uid;                     /* User ID */
    gid_t i_gid;                     /* Group ID */
    dev_t i_rdev;                    /* Real device node */
    loff_t i_size;                   /* File size */
    struct timespec i_atime;         /* Last access time */
    struct timespec i_mtime;         /* Last modify time */
    struct timespec i_ctime;         /* Last change time */
    unsigned int i_blkbits;          /* Block size bits */
    unsigned long i_blksize;         /* Block size */
    unsigned long i_blocks;          /* File size in blocks */
    unsigned long i_state;           /* State flags */
    struct mutex i_mutex;            /* Inode mutex */
    unsigned long i_flags;           /* Filesystem flags */
    unsigned int i_nlink;            /* Number of links */
    unsigned int i_count;            /* Reference count */
    unsigned int i_version;          /* Version number */
    union {
        struct pipe_inode_info *i_pipe; /* Pipe info */
        struct block_device *i_bdev;    /* Block device */
        struct cdev *i_cdev;            /* Character device */
    };
    struct list_head i_dentry;       /* Dentry list */
    struct list_head i_devices;      /* Device list */
    struct list_head i_wb_list;      /* Write-back list */
    struct list_head i_lru;          /* LRU list */
    struct list_head i_sb_list;      /* Super block list */
    union {
        struct hlist_head i_dentry_hlist; /* Dentry hash list */
        struct rcu_head i_rcu;           /* RCU head */
    };
    u64 i_version_queried;           /* Last queried version */
    struct address_space *i_mapping; /* Address space */
    struct address_space i_data;     /* Data */
    struct dquot *i_dquot[MAXQUOTAS]; /* Quota */
    struct list_head i_devices;      /* Device list */
    struct inode_operations *i_op;   /* Inode operations */
    struct file_operations *i_fop;   /* File operations */
    struct super_block *i_sb;        /* Super block */
    struct file_lock *i_flock;       /* File lock list */
    struct address_space i_mapping;  /* Address space */
    void *i_private;                 /* Private data */
} inode_t;

/* Directory entry structure */
typedef struct dentry {
    atomic_t d_count;                /* Reference count */
    unsigned int d_flags;            /* Dentry flags */
    spinlock_t d_lock;               /* Dentry lock */
    struct inode *d_inode;           /* Inode */
    struct hlist_node d_hash;        /* Hash list */
    struct dentry *d_parent;         /* Parent dentry */
    struct qstr d_name;              /* Dentry name */
    struct list_head d_lru;          /* LRU list */
    struct list_head d_child;        /* Child list */
    struct list_head d_subdirs;      /* Subdirectory list */
    struct list_head d_alias;        /* Alias list */
    unsigned long d_time;            /* Revalidate time */
    struct dentry_operations *d_op;  /* Dentry operations */
    struct super_block *d_sb;        /* Super block */
    void *d_fsdata;                  /* File system specific data */
    unsigned char d_iname[DNAME_INLINE_LEN]; /* Inline name */
} dentry_t;

/* File structure */
typedef struct file {
    struct list_head f_list;         /* File list */
    struct dentry *f_dentry;         /* Directory entry */
    struct vfsmount *f_vfsmnt;       /* VFS mount */
    struct file_operations *f_op;    /* File operations */
    atomic_t f_count;                /* Reference count */
    unsigned int f_flags;            /* File flags */
    mode_t f_mode;                   /* File mode */
    loff_t f_pos;                    /* File position */
    struct fown_struct f_owner;      /* Owner */
    unsigned int f_uid, f_gid;       /* User and group ID */
    struct file_ra_state f_ra;       /* Read-ahead state */
    u64 f_version;                   /* Version number */
    void *f_security;                /* Security */
    void *private_data;              /* Private data */
    struct list_head f_ep_links;     /* Event poll links */
    spinlock_t f_ep_lock;            /* Event poll lock */
    struct address_space *f_mapping; /* Address space mapping */
} file_t;

/* VFS mount structure */
typedef struct vfsmount {
    struct list_head mnt_hash;       /* Hash list */
    struct vfsmount *mnt_parent;     /* Parent mount */
    struct dentry *mnt_mountpoint;   /* Mount point */
    struct dentry *mnt_root;         /* Root dentry */
    struct super_block *mnt_sb;      /* Super block */
    struct list_head mnt_mounts;     /* Child mounts */
    struct list_head mnt_child;      /* Child list */
    int mnt_flags;                   /* Mount flags */
    const char *mnt_devname;         /* Device name */
    struct list_head mnt_list;       /* Mount list */
    struct list_head mnt_expire;     /* Expiration list */
    struct list_head mnt_share;      /* Shared list */
    struct list_head mnt_slave_list; /* Slave list */
    struct list_head mnt_slave;      /* Slave entry */
    struct vfsmount *mnt_master;     /* Master */
    struct mnt_namespace *mnt_ns;    /* Namespace */
    int mnt_id;                      /* Mount ID */
    int mnt_group_id;                /* Mount group ID */
    atomic_t mnt_count;              /* Reference count */
    int mnt_expiry_mark;             /* Expiry mark */
    int mnt_pinned;                  /* Pinned count */
    int mnt_ghosts;                  /* Ghost count */
} vfsmount_t;

/* VFS functions */
void vfs_init(void);
int vfs_mount(const char *source, const char *target, const char *fstype, unsigned long flags, void *data);
int vfs_umount(const char *target, int flags);
int vfs_open(const char *path, int flags, mode_t mode, file_t **file);
int vfs_close(file_t *file);
ssize_t vfs_read(file_t *file, void *buf, size_t count, loff_t *pos);
ssize_t vfs_write(file_t *file, const void *buf, size_t count, loff_t *pos);
int vfs_stat(const char *path, struct stat *buf);
int vfs_fstat(file_t *file, struct stat *buf);
int vfs_lstat(const char *path, struct stat *buf);
int vfs_mkdir(const char *path, mode_t mode);
int vfs_rmdir(const char *path);
int vfs_unlink(const char *path);
int vfs_rename(const char *oldpath, const char *newpath);
int vfs_link(const char *oldpath, const char *newpath);
int vfs_symlink(const char *oldpath, const char *newpath);
int vfs_readlink(const char *path, char *buf, size_t bufsiz);
int vfs_chmod(const char *path, mode_t mode);
int vfs_chown(const char *path, uid_t owner, gid_t group);
int vfs_utimes(const char *path, const struct timeval times[2]);
int vfs_access(const char *path, int mode);
int vfs_truncate(const char *path, loff_t length);
int vfs_ftruncate(file_t *file, loff_t length);
int vfs_fsync(file_t *file);
int vfs_fdatasync(file_t *file);
int vfs_statfs(const char *path, struct statfs *buf);
int vfs_fstatfs(file_t *file, struct statfs *buf);
int vfs_utime(const char *path, const struct utimbuf *times);
int vfs_mknod(const char *path, mode_t mode, dev_t dev);
int vfs_chdir(const char *path);
int vfs_fchdir(file_t *file);
char *vfs_getcwd(char *buf, size_t size);
int vfs_ioctl(file_t *file, unsigned int cmd, unsigned long arg);
int vfs_fcntl(file_t *file, unsigned int cmd, unsigned long arg);
int vfs_flock(file_t *file, unsigned int cmd);
int vfs_readdir(file_t *file, void *dirent, int (*filldir)(void *, const char *, int, loff_t, u64, unsigned));
int vfs_seek(file_t *file, loff_t offset, int whence);
int vfs_llseek(file_t *file, loff_t offset, int whence, loff_t *result);
int vfs_mmap(file_t *file, struct vm_area_struct *vma);
int vfs_register_filesystem(file_system_type_t *fs);
int vfs_unregister_filesystem(file_system_type_t *fs);
file_system_type_t *vfs_find_filesystem(const char *name);

#endif /* _KERNEL_VFS_H */
