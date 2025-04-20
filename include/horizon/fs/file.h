/**
 * file.h - Horizon kernel file operations definitions
 *
 * This file contains definitions for the file operations subsystem.
 * The definitions are compatible with Linux.
 */

#ifndef _KERNEL_FS_FILE_H
#define _KERNEL_FS_FILE_H

#include <horizon/types.h>
#include <horizon/list.h>

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
struct file;
struct inode;
struct dentry;
struct vfsmount;
struct super_block;
struct file_system_type;

/* File operations */
typedef struct file_operations {
    struct module *owner;
    loff_t (*llseek) (struct file *, loff_t, int);
    ssize_t (*read) (struct file *, char *, size_t, loff_t *);
    ssize_t (*write) (struct file *, const char *, size_t, loff_t *);
    ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
    ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
    int (*iterate) (struct file *, struct dir_context *);
    int (*iterate_shared) (struct file *, struct dir_context *);
    unsigned int (*poll) (struct file *, struct poll_table_struct *);
    long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
    long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
    int (*mmap) (struct file *, struct vm_area_struct *);
    int (*open) (struct inode *, struct file *);
    int (*flush) (struct file *, fl_owner_t id);
    int (*release) (struct inode *, struct file *);
    int (*fsync) (struct file *, loff_t, loff_t, int datasync);
    int (*fasync) (int, struct file *, int);
    int (*lock) (struct file *, int, struct file_lock *);
    ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
    unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
    int (*check_flags)(int);
    int (*flock) (struct file *, int, struct file_lock *);
    ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
    ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
    int (*setlease)(struct file *, long, struct file_lock **, void **);
    long (*fallocate)(struct file *file, int mode, loff_t offset, loff_t len);
    void (*show_fdinfo)(struct seq_file *m, struct file *f);
    unsigned (*mmap_capabilities)(struct file *);
    ssize_t (*copy_file_range)(struct file *, loff_t, struct file *, loff_t, size_t, unsigned int);
    int (*clone_file_range)(struct file *, loff_t, struct file *, loff_t, u64);
    ssize_t (*dedupe_file_range)(struct file *, u64, u64, struct file *, u64);
} file_operations_t;

/* Inode operations */
typedef struct inode_operations {
    struct dentry * (*lookup) (struct inode *, struct dentry *, unsigned int);
    const char * (*get_link) (struct dentry *, struct inode *, struct delayed_call *);
    int (*permission) (struct inode *, int);
    struct posix_acl * (*get_acl)(struct inode *, int);
    int (*readlink) (struct dentry *, char *, int);
    int (*create) (struct inode *, struct dentry *, umode_t, bool);
    int (*link) (struct dentry *, struct inode *, struct dentry *);
    int (*unlink) (struct inode *, struct dentry *);
    int (*symlink) (struct inode *, struct dentry *, const char *);
    int (*mkdir) (struct inode *, struct dentry *, umode_t);
    int (*rmdir) (struct inode *, struct dentry *);
    int (*mknod) (struct inode *, struct dentry *, umode_t, dev_t);
    int (*rename) (struct inode *, struct dentry *, struct inode *, struct dentry *, unsigned int);
    int (*setattr) (struct dentry *, struct iattr *);
    int (*getattr) (const struct path *, struct kstat *, u32, unsigned int);
    ssize_t (*listxattr) (struct dentry *, char *, size_t);
    int (*fiemap)(struct inode *, struct fiemap_extent_info *, u64 start, u64 len);
    int (*update_time)(struct inode *, struct timespec *, int);
    int (*atomic_open)(struct inode *, struct dentry *, struct file *, unsigned open_flag, umode_t create_mode);
    int (*tmpfile) (struct inode *, struct dentry *, umode_t);
    int (*set_acl)(struct inode *, struct posix_acl *, int);
} inode_operations_t;

/* Super operations */
typedef struct super_operations {
    struct inode *(*alloc_inode)(struct super_block *sb);
    void (*destroy_inode)(struct inode *);
    void (*dirty_inode) (struct inode *, int flags);
    int (*write_inode) (struct inode *, struct writeback_control *wbc);
    int (*drop_inode) (struct inode *);
    void (*evict_inode) (struct inode *);
    void (*put_super) (struct super_block *);
    int (*sync_fs)(struct super_block *sb, int wait);
    int (*freeze_super) (struct super_block *);
    int (*freeze_fs) (struct super_block *);
    int (*thaw_super) (struct super_block *);
    int (*unfreeze_fs) (struct super_block *);
    int (*statfs) (struct dentry *, struct kstatfs *);
    int (*remount_fs) (struct super_block *, int *, char *);
    void (*umount_begin) (struct super_block *);
    int (*show_options)(struct seq_file *, struct dentry *);
    int (*show_devname)(struct seq_file *, struct dentry *);
    int (*show_path)(struct seq_file *, struct dentry *);
    int (*show_stats)(struct seq_file *, struct dentry *);
    int (*bdev_try_to_free_page)(struct super_block*, struct page*, gfp_t);
    long (*nr_cached_objects)(struct super_block *);
    long (*free_cached_objects)(struct super_block *, long);
} super_operations_t;

/* File structure */
typedef struct file {
    struct path f_path;
    struct inode *f_inode;
    const struct file_operations *f_op;
    spinlock_t f_lock;
    atomic_long_t f_count;
    unsigned int f_flags;
    fmode_t f_mode;
    struct mutex f_pos_lock;
    loff_t f_pos;
    struct fown_struct f_owner;
    const struct cred *f_cred;
    struct file_ra_state f_ra;
    u64 f_version;
    void *f_security;
    void *private_data;
    struct list_head f_ep_links;
    struct list_head f_tfile_llink;
    struct address_space *f_mapping;
    errseq_t f_wb_err;
} file_t;

/* Inode structure */
typedef struct inode {
    umode_t i_mode;
    unsigned short i_opflags;
    kuid_t i_uid;
    kgid_t i_gid;
    unsigned int i_flags;
    struct posix_acl *i_acl;
    struct posix_acl *i_default_acl;
    const struct inode_operations *i_op;
    struct super_block *i_sb;
    struct address_space *i_mapping;
    void *i_security;
    unsigned long i_ino;
    union {
        const unsigned int i_nlink;
        unsigned int __i_nlink;
    };
    dev_t i_rdev;
    loff_t i_size;
    struct timespec i_atime;
    struct timespec i_mtime;
    struct timespec i_ctime;
    spinlock_t i_lock;
    unsigned short i_bytes;
    unsigned int i_blkbits;
    blkcnt_t i_blocks;
    unsigned long i_state;
    struct rw_semaphore i_rwsem;
    struct list_head i_wb_list;
    struct list_head i_lru;
    struct list_head i_sb_list;
    union {
        struct hlist_head i_dentry;
        struct rcu_head i_rcu;
    };
    u64 i_version;
    atomic_t i_count;
    atomic_t i_dio_count;
    atomic_t i_writecount;
    const struct file_operations *i_fop;
    struct file_lock_context *i_flctx;
    struct address_space i_data;
    struct list_head i_devices;
    union {
        struct pipe_inode_info *i_pipe;
        struct block_device *i_bdev;
        struct cdev *i_cdev;
        char *i_link;
        unsigned i_dir_seq;
    };
    __u32 i_generation;
    __u32 i_fsnotify_mask;
    struct fsnotify_mark_connector __rcu *i_fsnotify_marks;
    struct fscrypt_info *i_crypt_info;
    void *i_private;
} inode_t;

/* Dentry structure */
typedef struct dentry {
    unsigned int d_flags;
    seqcount_t d_seq;
    struct hlist_bl_node d_hash;
    struct dentry *d_parent;
    struct qstr d_name;
    struct inode *d_inode;
    unsigned char d_iname[DNAME_INLINE_LEN];
    struct lockref d_lockref;
    const struct dentry_operations *d_op;
    struct super_block *d_sb;
    unsigned long d_time;
    void *d_fsdata;
    struct list_head d_lru;
    struct list_head d_child;
    struct list_head d_subdirs;
    union {
        struct hlist_node d_alias;
        struct rcu_head d_rcu;
    } d_u;
} dentry_t;

/* Scatter/gather I/O vector structure */
struct iovec {
    void *iov_base;     /* Starting address */
    size_t iov_len;     /* Number of bytes to transfer */
};

/* File functions */
int file_open(const char *path, int flags, mode_t mode, file_t **file);
int file_close(file_t *file);
ssize_t file_read(file_t *file, void *buf, size_t count);
ssize_t file_write(file_t *file, const void *buf, size_t count);
int file_ioctl(file_t *file, unsigned int cmd, unsigned long arg);
int file_seek(file_t *file, off_t offset, int whence);
int file_truncate(file_t *file, off_t length);
int file_sync(file_t *file);
int file_stat(const char *path, struct stat *buf);
int file_fstat(file_t *file, struct stat *buf);
int file_lstat(const char *path, struct stat *buf);
int file_access(const char *path, int mode);
int file_chmod(const char *path, mode_t mode);
int file_chown(const char *path, uid_t owner, gid_t group);
int file_utime(const char *path, const struct utimbuf *times);
int file_mkdir(const char *path, mode_t mode);
int file_rmdir(const char *path);
int file_unlink(const char *path);
int file_rename(const char *oldpath, const char *newpath);
int file_link(const char *oldpath, const char *newpath);
int file_symlink(const char *oldpath, const char *newpath);
int file_readlink(const char *path, char *buf, size_t bufsiz);
int file_mknod(const char *path, mode_t mode, dev_t dev);
int file_mount(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data);
int file_umount(const char *target);
int file_statfs(const char *path, struct statfs *buf);
int file_fstatfs(file_t *file, struct statfs *buf);
int file_chdir(const char *path);
int file_fchdir(file_t *file);
char *file_getcwd(char *buf, size_t size);
int file_dup(file_t *file);
int file_dup2(file_t *file, int newfd);
int file_pipe(file_t **read_file, file_t **write_file);
int file_fcntl(file_t *file, int cmd, unsigned long arg);
int file_flock(file_t *file, int operation);
int file_readdir(file_t *file, struct dirent *dirp, unsigned int count);
int file_rewinddir(file_t *file);
int file_seekdir(file_t *file, long loc);
long file_telldir(file_t *file);
int file_mmap(file_t *file, void *addr, size_t length, int prot, int flags, off_t offset, void **mapped_addr);
int file_munmap(void *addr, size_t length);
int file_msync(void *addr, size_t length, int flags);
int file_mprotect(void *addr, size_t length, int prot);
int file_madvise(void *addr, size_t length, int advice);
int file_mincore(void *addr, size_t length, unsigned char *vec);
int file_mlock(void *addr, size_t length);
int file_munlock(void *addr, size_t length);
int file_mlockall(int flags);
int file_munlockall(void);
int file_remap_file_pages(void *addr, size_t size, int prot, size_t pgoff, int flags);
int file_sync_file_range(file_t *file, off_t offset, off_t nbytes, unsigned int flags);
int file_fallocate(file_t *file, int mode, off_t offset, off_t len);
int file_fadvise(file_t *file, off_t offset, off_t len, int advice);
int file_posix_fadvise(file_t *file, off_t offset, off_t len, int advice);
int file_readahead(file_t *file, off_t offset, size_t count);
ssize_t file_sendfile(file_t *out_file, file_t *in_file, off_t *offset, size_t count);
int file_splice(file_t *fd_in, loff_t *off_in, file_t *fd_out, loff_t *off_out, size_t len, unsigned int flags);
int file_tee(file_t *fd_in, file_t *fd_out, size_t len, unsigned int flags);
int file_vmsplice(file_t *fd, const struct iovec *iov, unsigned long nr_segs, unsigned int flags);
int file_copy_file_range(file_t *fd_in, loff_t *off_in, file_t *fd_out, loff_t *off_out, size_t len, unsigned int flags);
int file_ioctl_ficlone(file_t *dest_file, file_t *src_file);
int file_ioctl_ficlonerange(file_t *dest_file, struct file_clone_range *range);
int file_ioctl_fideduperange(file_t *src_file, struct file_dedupe_range *range);

/* Advanced I/O functions */
ssize_t file_readv(file_t *file, const struct iovec *iov, int iovcnt);
ssize_t file_writev(file_t *file, const struct iovec *iov, int iovcnt);
int file_fsync(file_t *file, int datasync);
int file_fdatasync(file_t *file);
int file_get_seals(file_t *file);
int file_set_seals(file_t *file, int seals);
off_t file_size(file_t *file);
int file_is_dir(file_t *file);
int file_is_regular(file_t *file);
int file_is_symlink(file_t *file);
int file_is_block_device(file_t *file);
int file_is_char_device(file_t *file);
int file_is_fifo(file_t *file);
int file_is_socket(file_t *file);

/* Extended attribute functions */
ssize_t file_getxattr(const char *path, const char *name, void *value, size_t size);
int file_setxattr(const char *path, const char *name, const void *value, size_t size, int flags);
ssize_t file_listxattr(const char *path, char *list, size_t size);
int file_removexattr(const char *path, const char *name);
ssize_t file_lgetxattr(const char *path, const char *name, void *value, size_t size);
int file_lsetxattr(const char *path, const char *name, const void *value, size_t size, int flags);
ssize_t file_llistxattr(const char *path, char *list, size_t size);
int file_lremovexattr(const char *path, const char *name);
ssize_t file_fgetxattr(file_t *file, const char *name, void *value, size_t size);
int file_fsetxattr(file_t *file, const char *name, const void *value, size_t size, int flags);
ssize_t file_flistxattr(file_t *file, char *list, size_t size);
int file_fremovexattr(file_t *file, const char *name);

/* File notification functions */
int inotify_init(void);
int inotify_init1(int flags);
int inotify_add_watch(int fd, const char *pathname, uint32_t mask);
int inotify_rm_watch(int fd, int wd);
void inotify_close(struct inotify_instance *instance);
ssize_t inotify_read(struct inotify_instance *instance, char *buffer, size_t count);
int inotify_add_event(struct inotify_instance *instance, int wd, uint32_t mask, uint32_t cookie, const char *name, size_t name_len);
void inotify_notify_event(struct path *path, uint32_t mask, uint32_t cookie, const char *name, size_t name_len);
void inotify_init_module(void);

#endif /* _KERNEL_FS_FILE_H */
