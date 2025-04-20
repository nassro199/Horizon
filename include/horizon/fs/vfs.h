/**
 * vfs.h - Horizon kernel Virtual File System definitions
 *
 * This file contains definitions for the Virtual File System (VFS) layer.
 * The definitions are compatible with Linux.
 */

#ifndef _KERNEL_FS_VFS_H
#define _KERNEL_FS_VFS_H

#include <horizon/types.h>
#include <horizon/list.h>
#include <horizon/fs/file.h>

/* Maximum path length */
#define PATH_MAX 4096

/* Maximum file name length */
#define NAME_MAX 255

/* Maximum number of symlink traversals */
#define MAX_SYMLINK_DEPTH 8

/* Mount flags */
#define MS_RDONLY      0x00000001  /* Mount read-only */
#define MS_NOSUID      0x00000002  /* Ignore suid and sgid bits */
#define MS_NODEV       0x00000004  /* Disallow access to device special files */
#define MS_NOEXEC      0x00000008  /* Disallow program execution */
#define MS_SYNCHRONOUS 0x00000010  /* Writes are synced at once */
#define MS_REMOUNT     0x00000020  /* Alter flags of a mounted FS */
#define MS_MANDLOCK    0x00000040  /* Allow mandatory locks on an FS */
#define MS_DIRSYNC     0x00000080  /* Directory modifications are synchronous */
#define MS_NOATIME     0x00000400  /* Do not update access times */
#define MS_NODIRATIME  0x00000800  /* Do not update directory access times */
#define MS_BIND        0x00001000  /* Bind directory at different place */
#define MS_MOVE        0x00002000  /* Move a subtree */
#define MS_REC         0x00004000  /* Recursive loopback */
#define MS_SILENT      0x00008000  /* Don't emit messages */
#define MS_POSIXACL    0x00010000  /* VFS does not apply the umask */
#define MS_UNBINDABLE  0x00020000  /* Change to unbindable */
#define MS_PRIVATE     0x00040000  /* Change to private */
#define MS_SLAVE       0x00080000  /* Change to slave */
#define MS_SHARED      0x00100000  /* Change to shared */
#define MS_RELATIME    0x00200000  /* Update atime relative to mtime/ctime */
#define MS_KERNMOUNT   0x00400000  /* This is a kern_mount call */
#define MS_I_VERSION   0x00800000  /* Update inode I_version field */
#define MS_STRICTATIME 0x01000000  /* Always perform atime updates */
#define MS_LAZYTIME    0x02000000  /* Update the on-disk [acm]times lazily */

/* Superblock flags */
#define SB_RDONLY      0x0001  /* Read-only filesystem */
#define SB_NOSUID      0x0002  /* Ignore suid and sgid bits */
#define SB_NODEV       0x0004  /* Disallow access to device special files */
#define SB_NOEXEC      0x0008  /* Disallow program execution */
#define SB_SYNCHRONOUS 0x0010  /* Writes are synced at once */
#define SB_MANDLOCK    0x0040  /* Allow mandatory locks on an FS */
#define SB_DIRSYNC     0x0080  /* Directory modifications are synchronous */
#define SB_NOATIME     0x0400  /* Do not update access times */
#define SB_NODIRATIME  0x0800  /* Do not update directory access times */
#define SB_POSIXACL    0x10000 /* VFS does not apply the umask */
#define SB_I_VERSION   0x80000 /* Update inode I_version field */
#define SB_LAZYTIME    0x200000 /* Update the on-disk [acm]times lazily */

/* Lookup flags */
#define LOOKUP_FOLLOW        0x0001  /* Follow links */
#define LOOKUP_DIRECTORY     0x0002  /* Must be a directory */
#define LOOKUP_CONTINUE      0x0004  /* Continue lookup */
#define LOOKUP_AUTOMOUNT     0x0008  /* Automount points */
#define LOOKUP_PARENT        0x0010  /* Look up parent */
#define LOOKUP_REVAL         0x0020  /* Check validity */
#define LOOKUP_RCU           0x0040  /* RCU walk */
#define LOOKUP_NO_REVAL      0x0080  /* No revalidation */
#define LOOKUP_OPEN          0x0100  /* Open file */
#define LOOKUP_CREATE        0x0200  /* Create file */
#define LOOKUP_EXCL          0x0400  /* Exclusive open */
#define LOOKUP_RENAME_TARGET 0x0800  /* Rename target */
#define LOOKUP_JUMPED        0x1000  /* Jumped to a different mount */
#define LOOKUP_ROOT          0x2000  /* Lookup relative to root */
#define LOOKUP_EMPTY         0x4000  /* Empty path */

/* Forward declarations */
struct vfsmount;
struct dentry;
struct inode;
struct file;
struct super_block;
struct file_system_type;
struct nameidata;
struct path;
struct qstr;
struct kstat;

/* Path structure */
typedef struct path {
    struct vfsmount *mnt;  /* Mount point */
    struct dentry *dentry; /* Dentry */
} path_t;

/* Qualified string */
typedef struct qstr {
    const unsigned char *name;  /* String name */
    unsigned int len;           /* String length */
    unsigned int hash;          /* String hash */
} qstr_t;

/* Name lookup data */
typedef struct nameidata {
    struct path path;           /* Current path */
    struct qstr last;           /* Last component */
    struct path root;           /* Root directory */
    struct inode *inode;        /* Current inode */
    unsigned int flags;         /* Lookup flags */
    unsigned int seq;           /* Sequence counter */
    int last_type;              /* Last component type */
    unsigned int depth;         /* Symlink depth */
    char *saved_names[MAX_SYMLINK_DEPTH]; /* Saved names */
} nameidata_t;

/* Directory context */
typedef struct dir_context {
    int (*actor)(struct dir_context *, const char *, int, loff_t, u64, unsigned int); /* Actor function */
    loff_t pos;                 /* Position */
} dir_context_t;

/* File system context */
typedef struct fs_context {
    struct file_system_type *fs_type; /* File system type */
    void *fs_private;           /* File system private data */
    void *sget_key;             /* Superblock key */
    struct dentry *root;        /* Root dentry */
    struct user_namespace *user_ns; /* User namespace */
    int sb_flags;               /* Superblock flags */
    int sb_flags_mask;          /* Superblock flags mask */
    const char *source;         /* Source */
    void *security;             /* Security */
} fs_context_t;

/* File system statistics */
typedef struct kstatfs {
    long f_type;                /* Type of filesystem */
    long f_bsize;               /* Optimal transfer block size */
    u64 f_blocks;               /* Total data blocks in filesystem */
    u64 f_bfree;                /* Free blocks in filesystem */
    u64 f_bavail;               /* Free blocks available to unprivileged user */
    u64 f_files;                /* Total file nodes in filesystem */
    u64 f_ffree;                /* Free file nodes in filesystem */
    __kernel_fsid_t f_fsid;     /* Filesystem id */
    long f_namelen;             /* Maximum length of filenames */
    long f_frsize;              /* Fragment size */
    long f_flags;               /* Mount flags */
    long f_spare[4];            /* Spare for later */
} kstatfs_t;

/* File system type */
typedef struct file_system_type {
    const char *name;           /* File system name */
    int fs_flags;               /* File system flags */
    struct dentry *(*mount)(struct file_system_type *, int, const char *, void *); /* Mount function */
    void (*kill_sb)(struct super_block *); /* Kill superblock function */
    struct module *owner;       /* Owner module */
    struct file_system_type *next; /* Next file system type */
    struct list_head fs_supers; /* Superblock list */
    struct lock_class_key s_lock_key; /* Lock class key */
    struct lock_class_key s_umount_key; /* Umount lock class key */
    struct lock_class_key s_vfs_rename_key; /* VFS rename lock class key */
    struct lock_class_key s_writers_key[SB_FREEZE_LEVELS]; /* Writers lock class keys */
    struct lock_class_key i_lock_key; /* Inode lock class key */
    struct lock_class_key i_mutex_key; /* Inode mutex class key */
    struct lock_class_key i_mutex_dir_key; /* Inode directory mutex class key */
} file_system_type_t;

/* Mount structure */
typedef struct vfsmount {
    struct dentry *mnt_root;    /* Root dentry */
    struct super_block *mnt_sb; /* Superblock */
    int mnt_flags;              /* Mount flags */
    struct list_head mnt_list;  /* Mount list */
    struct list_head mnt_child; /* Child mounts */
    struct list_head mnt_mounts; /* Mounted filesystems */
    struct vfsmount *mnt_parent; /* Parent mount */
    struct dentry *mnt_mountpoint; /* Mount point */
    const char *mnt_devname;    /* Device name */
    struct list_head mnt_instance; /* Instance list */
} vfsmount_t;

/* VFS functions */
int vfs_open(const struct path *path, struct file **filp, int flags, umode_t mode);
int vfs_close(struct file *filp);
ssize_t vfs_read(struct file *filp, char __user *buf, size_t count, loff_t *pos);
ssize_t vfs_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos);
int vfs_readdir(struct file *filp, struct dir_context *ctx);
int vfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode);
int vfs_rmdir(struct inode *dir, struct dentry *dentry);
int vfs_unlink(struct inode *dir, struct dentry *dentry);
int vfs_rename(struct inode *old_dir, struct dentry *old_dentry, struct inode *new_dir, struct dentry *new_dentry, unsigned int flags);
int vfs_symlink(struct inode *dir, struct dentry *dentry, const char *oldname);
int vfs_link(struct dentry *old_dentry, struct inode *dir, struct dentry *new_dentry);
int vfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev);
int vfs_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool want_excl);
int vfs_truncate(struct path *path, loff_t length);
int vfs_create_file(const char *pathname, mode_t mode, struct path *path);
int vfs_permission(const struct path *path, int mode);
struct dentry *vfs_create_dentry(struct dentry *parent, const char *name);
void vfs_free_dentry(struct dentry *dentry);
int vfs_d_path(const struct path *path, char *buf, int buflen);
int vfs_fallocate(struct file *filp, int mode, loff_t offset, loff_t len);
int vfs_getattr(const struct path *path, struct kstat *stat, u32 request_mask, unsigned int flags);
int vfs_setattr(struct dentry *dentry, struct iattr *attr);
int vfs_statfs(struct dentry *dentry, struct kstatfs *buf);
int vfs_fsync(struct file *filp, int datasync);
int vfs_utimes(const struct path *path, struct timespec *times);
int vfs_access(const struct path *path, int mode);
int vfs_readlink(struct dentry *dentry, char __user *buffer, int buflen);
int vfs_follow_link(struct nameidata *nd, const char *link);
int vfs_path_lookup(struct dentry *dentry, struct vfsmount *mnt, const char *name, unsigned int flags, struct path *path);
int vfs_kern_path(const char *name, unsigned int flags, struct path *path);
void vfs_path_release(struct path *path);
int vfs_getxattr(struct dentry *dentry, const char *name, void *value, size_t size);
int vfs_setxattr(struct dentry *dentry, const char *name, const void *value, size_t size, int flags);
int vfs_removexattr(struct dentry *dentry, const char *name);
ssize_t vfs_listxattr(struct dentry *dentry, char *list, size_t size);
int vfs_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int vfs_lock(struct file *filp, int cmd, struct file_lock *fl);
int vfs_flock(struct file *filp, int cmd, struct file_lock *fl);
int vfs_chown(struct path *path, uid_t user, gid_t group);
int vfs_chmod(struct path *path, umode_t mode);
int vfs_mmap(struct file *filp, struct vm_area_struct *vma);
int vfs_splice_to(struct file *in, loff_t *ppos, struct pipe_inode_info *pipe, size_t len, unsigned int flags);
int vfs_splice_from(struct pipe_inode_info *pipe, struct file *out, loff_t *ppos, size_t len, unsigned int flags);
ssize_t vfs_copy_file_range(struct file *file_in, loff_t pos_in, struct file *file_out, loff_t pos_out, size_t len, unsigned int flags);
int vfs_clone_file_range(struct file *file_in, loff_t pos_in, struct file *file_out, loff_t pos_out, u64 len);
ssize_t vfs_dedupe_file_range(struct file *file, struct file_dedupe_range *range);

/* Mount functions */
struct vfsmount *vfs_kern_mount(struct file_system_type *type, int flags, const char *name, void *data);
void vfs_kern_umount(struct vfsmount *mnt);
int vfs_mount(const char *dev_name, const char *dir_name, const char *type, unsigned long flags, void *data);
int vfs_umount(const char *name, int flags);
int vfs_statfs_by_dentry(struct dentry *dentry, struct kstatfs *buf);
int vfs_statfs_by_path(const char *pathname, struct kstatfs *buf);

/* File system registration */
int register_filesystem(struct file_system_type *fs);
int unregister_filesystem(struct file_system_type *fs);
struct file_system_type *get_fs_type(const char *name);

/* Path functions */
int kern_path(const char *name, unsigned int flags, struct path *path);
void path_put(struct path *path);
char *d_path(const struct path *path, char *buf, int buflen);
char *file_path(struct file *filp, char *buf, int buflen);

/* Dentry cache functions */
struct dentry *d_alloc(struct dentry *parent, const struct qstr *name);
struct dentry *d_alloc_root(struct inode *root_inode);
void d_add(struct dentry *dentry, struct inode *inode);
void d_delete(struct dentry *dentry);
void d_drop(struct dentry *dentry);
void d_rehash(struct dentry *dentry);
void d_invalidate(struct dentry *dentry);
struct dentry *d_lookup(struct dentry *parent, struct qstr *name);
int d_validate(struct dentry *dentry, struct dentry *parent);
int d_hash(const struct dentry *dentry, struct qstr *str);
int d_compare(const struct dentry *parent, const struct dentry *dentry, unsigned int len, const char *str, const struct qstr *name);
int simple_positive(struct dentry *dentry);
int simple_empty(struct dentry *dentry);
int simple_unlink(struct inode *dir, struct dentry *dentry);
int simple_rmdir(struct inode *dir, struct dentry *dentry);
int simple_rename(struct inode *old_dir, struct dentry *old_dentry, struct inode *new_dir, struct dentry *new_dentry);
int simple_setattr(struct dentry *dentry, struct iattr *attr);
int simple_getattr(const struct path *path, struct kstat *stat, u32 request_mask, unsigned int flags);
int simple_statfs(struct dentry *dentry, struct kstatfs *buf);
int simple_link(struct dentry *old_dentry, struct inode *dir, struct dentry *dentry);
int simple_setsize(struct inode *inode, loff_t size);
int simple_readpage(struct file *file, struct page *page);
int simple_write_begin(struct file *file, struct address_space *mapping, loff_t pos, unsigned len, unsigned flags, struct page **pagep, void **fsdata);
int simple_write_end(struct file *file, struct address_space *mapping, loff_t pos, unsigned len, unsigned copied, struct page *page, void *fsdata);
int simple_fill_super(struct super_block *sb, unsigned long magic, struct tree_descr *files);
int simple_pin_fs(struct file_system_type *fs_type, struct vfsmount **mount, int *count);
void simple_release_fs(struct vfsmount **mount, int *count);

/* Inode functions */
struct inode *new_inode(struct super_block *sb);
struct inode *iget_locked(struct super_block *sb, unsigned long ino);
void iget_failed(struct inode *inode);
void unlock_new_inode(struct inode *inode);
void iput(struct inode *inode);
void ihold(struct inode *inode);
void clear_inode(struct inode *inode);
void inc_nlink(struct inode *inode);
void drop_nlink(struct inode *inode);
void clear_nlink(struct inode *inode);
void set_nlink(struct inode *inode, unsigned int nlink);
void inc_iversion(struct inode *inode);
void inode_init_owner(struct inode *inode, const struct inode *dir, umode_t mode);
void init_special_inode(struct inode *inode, umode_t mode, dev_t dev);
void inode_init_once(struct inode *inode);
int inode_init_always(struct super_block *sb, struct inode *inode);
void inode_add_to_lists(struct super_block *sb, struct inode *inode);
void inode_sb_list_add(struct inode *inode);
void inode_dio_wait(struct inode *inode);
void inode_set_flags(struct inode *inode, unsigned int flags, unsigned int mask);
void inode_nohighmem(struct inode *inode);
void inode_set_gid(struct inode *inode, gid_t gid);
void inode_set_uid(struct inode *inode, uid_t uid);
void inode_set_atime(struct inode *inode, struct timespec *time);
void inode_set_mtime(struct inode *inode, struct timespec *time);
void inode_set_ctime(struct inode *inode, struct timespec *time);
void touch_atime(const struct path *path);
void file_update_time(struct file *file);
int inode_needs_sync(struct inode *inode);
int inode_wait(struct inode *inode);
int generic_delete_inode(struct inode *inode);
int generic_drop_inode(struct inode *inode);
int generic_detach_inode(struct inode *inode);

/* Superblock functions */
struct super_block *sget(struct file_system_type *type, int (*test)(struct super_block *, void *), int (*set)(struct super_block *, void *), int flags, void *data);
void kill_block_super(struct super_block *sb);
void kill_anon_super(struct super_block *sb);
void kill_litter_super(struct super_block *sb);
void deactivate_super(struct super_block *sb);
void deactivate_locked_super(struct super_block *sb);
int set_anon_super(struct super_block *sb, void *data);
int get_sb_pseudo(struct file_system_type *fs_type, char *name, const struct super_operations *ops, unsigned long magic, struct vfsmount *mnt);
int simple_set_mnt(struct vfsmount *mnt, struct super_block *sb);
int super_setup_bdi(struct super_block *sb);
void super_setup_bdi_name(struct super_block *sb, char *fmt, ...);

/* File operations */
struct file *alloc_file(struct path *path, fmode_t mode, const struct file_operations *fop);
struct file *alloc_file_pseudo(struct inode *inode, struct vfsmount *mnt, const char *name, int flags, const struct file_operations *fop);
struct file *get_empty_filp(void);
void put_filp(struct file *file);
void file_move(struct file *file, struct list_head *list);
void file_kill(struct file *file);
void file_sb_list_add(struct file *file, struct super_block *sb);
int file_sb_list_del(struct file *file);
int file_truncate(struct file *file, loff_t size);
int file_open(struct file *file, struct inode *inode);
int file_release(struct file *file);
loff_t file_llseek(struct file *file, loff_t offset, int whence);
ssize_t file_read(struct file *file, char __user *buf, size_t count, loff_t *pos);
ssize_t file_write(struct file *file, const char __user *buf, size_t count, loff_t *pos);
int file_readdir(struct file *file, struct dir_context *ctx);
int file_fsync(struct file *file, int datasync);
int file_mmap(struct file *file, struct vm_area_struct *vma);
int file_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
int file_flock(struct file *file, int cmd, struct file_lock *fl);
int file_sendpage(struct file *file, struct page *page, int offset, size_t size, loff_t *pos, int more);
ssize_t file_splice_read(struct file *file, loff_t *ppos, struct pipe_inode_info *pipe, size_t len, unsigned int flags);
ssize_t file_splice_write(struct pipe_inode_info *pipe, struct file *file, loff_t *ppos, size_t len, unsigned int flags);
int file_setlease(struct file *file, long arg, struct file_lock **lease, void **priv);
int file_fallocate(struct file *file, int mode, loff_t offset, loff_t len);
int file_dedupe_range(struct file *file, struct file_dedupe_range *range);
int file_clone_range(struct file *file_in, loff_t pos_in, struct file *file_out, loff_t pos_out, u64 len);
ssize_t file_copy_file_range(struct file *file_in, loff_t pos_in, struct file *file_out, loff_t pos_out, size_t len, unsigned int flags);

/* File descriptor functions */
int get_unused_fd_flags(unsigned flags);
void put_unused_fd(unsigned int fd);
struct file *fget(unsigned int fd);
struct file *fget_raw(unsigned int fd);
void fput(struct file *file);
int fd_install(unsigned int fd, struct file *file);
void fd_uninstall(unsigned int fd);
int close_fd(unsigned int fd);
int close_on_exec(unsigned int fd);
void set_close_on_exec(unsigned int fd, int flag);
bool get_close_on_exec(unsigned int fd);
int flush_all_files(void);
int iterate_fd(struct files_struct *files, unsigned start, int (*f)(const void *, struct file *, unsigned), const void *p);

/* External variables */
extern struct list_head super_blocks;
extern struct file_system_type *file_systems;
extern struct vfsmount *root_mnt;
extern struct dentry *root_dentry;
extern struct inode *root_inode;

#endif /* _KERNEL_FS_VFS_H */
