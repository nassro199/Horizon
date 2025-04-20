/**
 * ext2.h - Horizon kernel EXT2 file system definitions
 * 
 * This file contains definitions for the EXT2 file system.
 * The definitions are compatible with Linux.
 */

#ifndef _KERNEL_EXT2_H
#define _KERNEL_EXT2_H

#include <horizon/types.h>
#include <horizon/vfs.h>

/* EXT2 magic number */
#define EXT2_SUPER_MAGIC 0xEF53

/* EXT2 revision levels */
#define EXT2_GOOD_OLD_REV 0  /* Good old revision */
#define EXT2_DYNAMIC_REV  1  /* Dynamic revision */

/* EXT2 feature flags */
#define EXT2_FEATURE_COMPAT_DIR_PREALLOC   0x0001  /* Directory preallocation */
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES  0x0002  /* Imagic inodes */
#define EXT2_FEATURE_COMPAT_HAS_JOURNAL    0x0004  /* Has journal */
#define EXT2_FEATURE_COMPAT_EXT_ATTR       0x0008  /* Extended attributes */
#define EXT2_FEATURE_COMPAT_RESIZE_INO     0x0010  /* Resize inode */
#define EXT2_FEATURE_COMPAT_DIR_INDEX      0x0020  /* Directory index */

#define EXT2_FEATURE_INCOMPAT_COMPRESSION  0x0001  /* Compression */
#define EXT2_FEATURE_INCOMPAT_FILETYPE     0x0002  /* File type */
#define EXT2_FEATURE_INCOMPAT_RECOVER      0x0004  /* Needs recovery */
#define EXT2_FEATURE_INCOMPAT_JOURNAL_DEV  0x0008  /* Journal device */
#define EXT2_FEATURE_INCOMPAT_META_BG      0x0010  /* Meta block groups */

#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001 /* Sparse superblock */
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE   0x0002 /* Large file support */
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR    0x0004 /* Binary tree directories */

/* EXT2 file system states */
#define EXT2_VALID_FS 0x0001  /* Unmounted cleanly */
#define EXT2_ERROR_FS 0x0002  /* Errors detected */

/* EXT2 error handling methods */
#define EXT2_ERRORS_CONTINUE 1  /* Continue as if nothing happened */
#define EXT2_ERRORS_RO       2  /* Remount read-only */
#define EXT2_ERRORS_PANIC    3  /* Cause a kernel panic */

/* EXT2 creator operating systems */
#define EXT2_OS_LINUX   0  /* Linux */
#define EXT2_OS_HURD    1  /* GNU HURD */
#define EXT2_OS_MASIX   2  /* MASIX */
#define EXT2_OS_FREEBSD 3  /* FreeBSD */
#define EXT2_OS_LITES   4  /* Lites */

/* EXT2 inode types */
#define EXT2_S_IFMT   0xF000  /* Format mask */
#define EXT2_S_IFSOCK 0xC000  /* Socket */
#define EXT2_S_IFLNK  0xA000  /* Symbolic link */
#define EXT2_S_IFREG  0x8000  /* Regular file */
#define EXT2_S_IFBLK  0x6000  /* Block device */
#define EXT2_S_IFDIR  0x4000  /* Directory */
#define EXT2_S_IFCHR  0x2000  /* Character device */
#define EXT2_S_IFIFO  0x1000  /* FIFO */

/* EXT2 inode flags */
#define EXT2_SECRM_FL        0x00000001  /* Secure deletion */
#define EXT2_UNRM_FL         0x00000002  /* Undelete */
#define EXT2_COMPR_FL        0x00000004  /* Compress file */
#define EXT2_SYNC_FL         0x00000008  /* Synchronous updates */
#define EXT2_IMMUTABLE_FL    0x00000010  /* Immutable file */
#define EXT2_APPEND_FL       0x00000020  /* Append only */
#define EXT2_NODUMP_FL       0x00000040  /* Do not dump file */
#define EXT2_NOATIME_FL      0x00000080  /* Do not update atime */
#define EXT2_DIRTY_FL        0x00000100  /* Dirty */
#define EXT2_COMPRBLK_FL     0x00000200  /* Compressed blocks */
#define EXT2_NOCOMPR_FL      0x00000400  /* Access raw compressed data */
#define EXT2_ECOMPR_FL       0x00000800  /* Compression error */
#define EXT2_BTREE_FL        0x00001000  /* B-tree format directory */
#define EXT2_INDEX_FL        0x00001000  /* Hash-indexed directory */
#define EXT2_IMAGIC_FL       0x00002000  /* AFS directory */
#define EXT2_JOURNAL_DATA_FL 0x00004000  /* Journal file data */
#define EXT2_NOTAIL_FL       0x00008000  /* File tail should not be merged */
#define EXT2_DIRSYNC_FL      0x00010000  /* Directory synchronous updates */
#define EXT2_TOPDIR_FL       0x00020000  /* Top of directory hierarchy */
#define EXT2_RESERVED_FL     0x80000000  /* Reserved for ext2 library */

/* EXT2 directory entry file types */
#define EXT2_FT_UNKNOWN  0  /* Unknown file type */
#define EXT2_FT_REG_FILE 1  /* Regular file */
#define EXT2_FT_DIR      2  /* Directory */
#define EXT2_FT_CHRDEV   3  /* Character device */
#define EXT2_FT_BLKDEV   4  /* Block device */
#define EXT2_FT_FIFO     5  /* FIFO */
#define EXT2_FT_SOCK     6  /* Socket */
#define EXT2_FT_SYMLINK  7  /* Symbolic link */
#define EXT2_FT_MAX      8  /* Max file type */

/* EXT2 superblock structure */
typedef struct ext2_super_block {
    u32 s_inodes_count;         /* Inodes count */
    u32 s_blocks_count;         /* Blocks count */
    u32 s_r_blocks_count;       /* Reserved blocks count */
    u32 s_free_blocks_count;    /* Free blocks count */
    u32 s_free_inodes_count;    /* Free inodes count */
    u32 s_first_data_block;     /* First data block */
    u32 s_log_block_size;       /* Block size */
    u32 s_log_frag_size;        /* Fragment size */
    u32 s_blocks_per_group;     /* Blocks per group */
    u32 s_frags_per_group;      /* Fragments per group */
    u32 s_inodes_per_group;     /* Inodes per group */
    u32 s_mtime;                /* Mount time */
    u32 s_wtime;                /* Write time */
    u16 s_mnt_count;            /* Mount count */
    u16 s_max_mnt_count;        /* Maximal mount count */
    u16 s_magic;                /* Magic signature */
    u16 s_state;                /* File system state */
    u16 s_errors;               /* Behaviour when detecting errors */
    u16 s_minor_rev_level;      /* Minor revision level */
    u32 s_lastcheck;            /* Time of last check */
    u32 s_checkinterval;        /* Max. time between checks */
    u32 s_creator_os;           /* OS */
    u32 s_rev_level;            /* Revision level */
    u16 s_def_resuid;           /* Default uid for reserved blocks */
    u16 s_def_resgid;           /* Default gid for reserved blocks */
    
    /* EXT2 dynamic revision fields */
    u32 s_first_ino;            /* First non-reserved inode */
    u16 s_inode_size;           /* Size of inode structure */
    u16 s_block_group_nr;       /* Block group # of this superblock */
    u32 s_feature_compat;       /* Compatible feature set */
    u32 s_feature_incompat;     /* Incompatible feature set */
    u32 s_feature_ro_compat;    /* Read-only compatible feature set */
    u8  s_uuid[16];             /* 128-bit uuid for volume */
    char s_volume_name[16];     /* Volume name */
    char s_last_mounted[64];    /* Directory where last mounted */
    u32 s_algorithm_usage_bitmap; /* For compression */
    
    /* Performance hints */
    u8  s_prealloc_blocks;      /* # of blocks to try to preallocate */
    u8  s_prealloc_dir_blocks;  /* # to preallocate for dirs */
    u16 s_padding1;             /* Padding to 4 bytes */
    
    /* Journaling support */
    u8  s_journal_uuid[16];     /* UUID of journal superblock */
    u32 s_journal_inum;         /* Inode number of journal file */
    u32 s_journal_dev;          /* Device number of journal file */
    u32 s_last_orphan;          /* Start of list of inodes to delete */
    
    /* Directory indexing support */
    u32 s_hash_seed[4];         /* HTREE hash seed */
    u8  s_def_hash_version;     /* Default hash version to use */
    u8  s_reserved_char_pad;    /* Padding */
    u16 s_reserved_word_pad;    /* Padding */
    
    /* Other options */
    u32 s_default_mount_opts;   /* Default mount options */
    u32 s_first_meta_bg;        /* First metablock block group */
    u32 s_reserved[190];        /* Padding to the end of the block */
} ext2_super_block_t;

/* EXT2 group descriptor structure */
typedef struct ext2_group_desc {
    u32 bg_block_bitmap;        /* Blocks bitmap block */
    u32 bg_inode_bitmap;        /* Inodes bitmap block */
    u32 bg_inode_table;         /* Inodes table block */
    u16 bg_free_blocks_count;   /* Free blocks count */
    u16 bg_free_inodes_count;   /* Free inodes count */
    u16 bg_used_dirs_count;     /* Directories count */
    u16 bg_pad;                 /* Padding */
    u32 bg_reserved[3];         /* Reserved */
} ext2_group_desc_t;

/* EXT2 inode structure */
typedef struct ext2_inode {
    u16 i_mode;                 /* File mode */
    u16 i_uid;                  /* Low 16 bits of owner uid */
    u32 i_size;                 /* Size in bytes */
    u32 i_atime;                /* Access time */
    u32 i_ctime;                /* Creation time */
    u32 i_mtime;                /* Modification time */
    u32 i_dtime;                /* Deletion time */
    u16 i_gid;                  /* Low 16 bits of group id */
    u16 i_links_count;          /* Links count */
    u32 i_blocks;               /* Blocks count */
    u32 i_flags;                /* File flags */
    u32 i_osd1;                 /* OS dependent 1 */
    u32 i_block[15];            /* Pointers to blocks */
    u32 i_generation;           /* File version (for NFS) */
    u32 i_file_acl;             /* File ACL */
    u32 i_dir_acl;              /* Directory ACL */
    u32 i_faddr;                /* Fragment address */
    u8  i_osd2[12];             /* OS dependent 2 */
} ext2_inode_t;

/* EXT2 directory entry structure */
typedef struct ext2_dir_entry {
    u32 inode;                  /* Inode number */
    u16 rec_len;                /* Directory entry length */
    u8  name_len;               /* Name length */
    u8  file_type;              /* File type */
    char name[255];             /* File name */
} ext2_dir_entry_t;

/* EXT2 file system information */
typedef struct ext2_fs_info {
    ext2_super_block_t *sb;     /* Superblock */
    ext2_group_desc_t *gd;      /* Group descriptors */
    u32 block_size;             /* Block size */
    u32 inodes_per_block;       /* Inodes per block */
    u32 blocks_per_group;       /* Blocks per group */
    u32 inodes_per_group;       /* Inodes per group */
    u32 groups_count;           /* Number of groups */
    u32 first_data_block;       /* First data block */
} ext2_fs_info_t;

/* EXT2 file system operations */
extern super_operations_t ext2_super_operations;
extern inode_operations_t ext2_inode_operations;
extern inode_operations_t ext2_dir_inode_operations;
extern inode_operations_t ext2_symlink_inode_operations;
extern file_operations_t ext2_file_operations;
extern file_operations_t ext2_dir_operations;
extern dentry_operations_t ext2_dentry_operations;
extern file_system_type_t ext2_fs_type;

/* EXT2 functions */
int ext2_init(void);
int ext2_read_super(super_block_t *sb, void *data, int silent);
int ext2_write_super(super_block_t *sb);
void ext2_put_super(super_block_t *sb);
int ext2_remount(super_block_t *sb, int *flags, char *data);
int ext2_statfs(super_block_t *sb, struct statfs *buf);
int ext2_read_inode(inode_t *inode);
int ext2_write_inode(inode_t *inode, int wait);
void ext2_delete_inode(inode_t *inode);
void ext2_clear_inode(inode_t *inode);
int ext2_sync_fs(super_block_t *sb, int wait);
int ext2_freeze_fs(super_block_t *sb);
int ext2_unfreeze_fs(super_block_t *sb);
int ext2_create(inode_t *dir, dentry_t *dentry, u32 mode, struct nameidata *nd);
struct dentry *ext2_lookup(inode_t *dir, dentry_t *dentry, struct nameidata *nd);
int ext2_link(dentry_t *old_dentry, inode_t *dir, dentry_t *dentry);
int ext2_unlink(inode_t *dir, dentry_t *dentry);
int ext2_symlink(inode_t *dir, dentry_t *dentry, const char *symname);
int ext2_mkdir(inode_t *dir, dentry_t *dentry, u32 mode);
int ext2_rmdir(inode_t *dir, dentry_t *dentry);
int ext2_mknod(inode_t *dir, dentry_t *dentry, u32 mode, dev_t dev);
int ext2_rename(inode_t *old_dir, dentry_t *old_dentry, inode_t *new_dir, dentry_t *new_dentry);
int ext2_readlink(dentry_t *dentry, char *buffer, int buflen);
void *ext2_follow_link(dentry_t *dentry, struct nameidata *nd);
void ext2_put_link(dentry_t *dentry, struct nameidata *nd, void *cookie);
int ext2_permission(inode_t *inode, int mask);
int ext2_setattr(dentry_t *dentry, struct iattr *attr);
int ext2_getattr(struct vfsmount *mnt, dentry_t *dentry, struct kstat *stat);
int ext2_open(inode_t *inode, file_t *file);
int ext2_release(inode_t *inode, file_t *file);
ssize_t ext2_read(file_t *file, char *buf, size_t count, loff_t *pos);
ssize_t ext2_write(file_t *file, const char *buf, size_t count, loff_t *pos);
int ext2_flush(file_t *file);
int ext2_fsync(file_t *file, dentry_t *dentry, int datasync);
loff_t ext2_llseek(file_t *file, loff_t offset, int whence);
int ext2_readdir(file_t *file, void *dirent, int (*filldir)(void *, const char *, int, loff_t, u64, unsigned));
int ext2_ioctl(inode_t *inode, file_t *file, unsigned int cmd, unsigned long arg);
int ext2_mmap(file_t *file, struct vm_area_struct *vma);
int ext2_check_flags(int flags);
int ext2_flock(file_t *file, int cmd, struct file_lock *lock);
ssize_t ext2_splice_write(struct pipe_inode_info *pipe, file_t *out, loff_t *ppos, size_t len, unsigned int flags);
ssize_t ext2_splice_read(file_t *in, loff_t *ppos, struct pipe_inode_info *pipe, size_t len, unsigned int flags);
int ext2_setlease(file_t *file, long arg, struct file_lock **lease);

#endif /* _KERNEL_EXT2_H */
