/**
 * ext2.h - Ext2 file system definitions
 *
 * This file contains definitions for the Ext2 file system.
 */

#ifndef _HORIZON_FS_EXT2_H
#define _HORIZON_FS_EXT2_H

#include <horizon/types.h>
#include <horizon/fs.h>

/* Ext2 magic number */
#define EXT2_MAGIC 0xEF53

/* Ext2 superblock structure */
typedef struct ext2_superblock {
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
    /* EXT2_DYNAMIC_REV superblocks only */
    u32 s_first_ino;            /* First non-reserved inode */
    u16 s_inode_size;           /* Size of inode structure */
    u16 s_block_group_nr;       /* Block group # of this superblock */
    u32 s_feature_compat;       /* Compatible feature set */
    u32 s_feature_incompat;     /* Incompatible feature set */
    u32 s_feature_ro_compat;    /* Readonly-compatible feature set */
    u8  s_uuid[16];             /* 128-bit uuid for volume */
    char s_volume_name[16];     /* Volume name */
    char s_last_mounted[64];    /* Directory where last mounted */
    u32 s_algorithm_usage_bitmap; /* For compression */
    /* Performance hints */
    u8  s_prealloc_blocks;      /* # of blocks to try to preallocate */
    u8  s_prealloc_dir_blocks;  /* # of blocks to preallocate for dirs */
    u16 s_padding1;
    /* Journaling support */
    u8  s_journal_uuid[16];     /* UUID of journal superblock */
    u32 s_journal_inum;         /* Inode number of journal file */
    u32 s_journal_dev;          /* Device number of journal file */
    u32 s_last_orphan;          /* Start of list of inodes to delete */
    u32 s_hash_seed[4];         /* HTREE hash seed */
    u8  s_def_hash_version;     /* Default hash version to use */
    u8  s_reserved_char_pad;
    u16 s_reserved_word_pad;
    u32 s_default_mount_opts;
    u32 s_first_meta_bg;        /* First metablock block group */
    u32 s_reserved[190];        /* Padding to the end of the block */
} ext2_superblock_t;

/* Ext2 block group descriptor structure */
typedef struct ext2_group_desc {
    u32 bg_block_bitmap;        /* Blocks bitmap block */
    u32 bg_inode_bitmap;        /* Inodes bitmap block */
    u32 bg_inode_table;         /* Inodes table block */
    u16 bg_free_blocks_count;   /* Free blocks count */
    u16 bg_free_inodes_count;   /* Free inodes count */
    u16 bg_used_dirs_count;     /* Directories count */
    u16 bg_pad;
    u32 bg_reserved[3];
} ext2_group_desc_t;

/* Ext2 inode structure */
typedef struct ext2_inode {
    u16 i_mode;                 /* File mode */
    u16 i_uid;                  /* Low 16 bits of Owner Uid */
    u32 i_size;                 /* Size in bytes */
    u32 i_atime;                /* Access time */
    u32 i_ctime;                /* Creation time */
    u32 i_mtime;                /* Modification time */
    u32 i_dtime;                /* Deletion Time */
    u16 i_gid;                  /* Low 16 bits of Group Id */
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

/* Ext2 directory entry structure */
typedef struct ext2_dir_entry {
    u32 inode;                  /* Inode number */
    u16 rec_len;                /* Directory entry length */
    u8  name_len;               /* Name length */
    u8  file_type;              /* File type */
    char name[255];             /* File name */
} ext2_dir_entry_t;

/* Ext2 file types */
#define EXT2_FT_UNKNOWN  0
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR      2
#define EXT2_FT_CHRDEV   3
#define EXT2_FT_BLKDEV   4
#define EXT2_FT_FIFO     5
#define EXT2_FT_SOCK     6
#define EXT2_FT_SYMLINK  7

/* Ext2 file modes */
#define EXT2_S_IFMT   0xF000    /* Format mask */
#define EXT2_S_IFSOCK 0xC000    /* Socket */
#define EXT2_S_IFLNK  0xA000    /* Symbolic link */
#define EXT2_S_IFREG  0x8000    /* Regular file */
#define EXT2_S_IFBLK  0x6000    /* Block device */
#define EXT2_S_IFDIR  0x4000    /* Directory */
#define EXT2_S_IFCHR  0x2000    /* Character device */
#define EXT2_S_IFIFO  0x1000    /* FIFO */
#define EXT2_S_ISUID  0x0800    /* Set user ID */
#define EXT2_S_ISGID  0x0400    /* Set group ID */
#define EXT2_S_ISVTX  0x0200    /* Sticky bit */
#define EXT2_S_IRUSR  0x0100    /* User read */
#define EXT2_S_IWUSR  0x0080    /* User write */
#define EXT2_S_IXUSR  0x0040    /* User execute */
#define EXT2_S_IRGRP  0x0020    /* Group read */
#define EXT2_S_IWGRP  0x0010    /* Group write */
#define EXT2_S_IXGRP  0x0008    /* Group execute */
#define EXT2_S_IROTH  0x0004    /* Others read */
#define EXT2_S_IWOTH  0x0002    /* Others write */
#define EXT2_S_IXOTH  0x0001    /* Others execute */

/* Ext2 in-memory superblock */
typedef struct ext2_sb_info {
    ext2_superblock_t *s_es;           /* Ext2 superblock */
    ext2_group_desc_t *s_group_desc;   /* Group descriptors */
    u32 s_block_size;                  /* Block size */
    u32 s_inodes_per_block;            /* Number of inodes per block */
    u32 s_blocks_per_group;            /* Number of blocks per group */
    u32 s_inodes_per_group;            /* Number of inodes per group */
    u32 s_itb_per_group;               /* Number of inode table blocks per group */
    u32 s_desc_per_block;              /* Number of group descriptors per block */
    u32 s_groups_count;                /* Number of groups */
    u32 s_first_data_block;            /* First data block */
    u32 s_first_ino;                   /* First non-reserved inode */
    u32 s_inode_size;                  /* Size of inode structure */
    void *s_blockdev;                  /* Block device */
} ext2_sb_info_t;

/* Ext2 in-memory inode */
typedef struct ext2_inode_info {
    ext2_inode_t *i_e2i;               /* Ext2 inode */
    u32 i_block_group;                 /* Block group */
    u32 i_data[15];                    /* Data blocks */
    u32 i_flags;                       /* Flags */
    u32 i_faddr;                       /* Fragment address */
    u8  i_frag_no;                     /* Fragment number */
    u8  i_frag_size;                   /* Fragment size */
    u16 i_state;                       /* State flags */
    u32 i_file_acl;                    /* File ACL */
    u32 i_dir_acl;                     /* Directory ACL */
    u32 i_dtime;                       /* Deletion time */
} ext2_inode_info_t;

/* Forward declarations for ext2 operations */
extern struct inode_operations ext2_inode_ops;
extern struct file_operations ext2_file_ops;
extern struct file_operations ext2_dir_ops;
extern struct super_operations ext2_super_ops;

/* Ext2 file system functions */
int ext2_init(void);
int ext2_mount(const char *dev, const char *dir, u32 flags);
int ext2_unmount(const char *dir);
file_t *ext2_open(const char *path, u32 flags);
error_t ext2_close(file_t *file);
ssize_t ext2_read(file_t *file, void *buffer, size_t size);
ssize_t ext2_write(file_t *file, const void *buffer, size_t size);
error_t ext2_seek(file_t *file, u64 offset, int whence);
error_t ext2_flush(file_t *file);
error_t ext2_fsync(file_t *file);
error_t ext2_truncate(const char *path, u64 size);
error_t ext2_ftruncate(file_t *file, u64 size);
error_t ext2_stat(const char *path, struct stat *buf);
error_t ext2_fstat(file_t *file, struct stat *buf);
error_t ext2_mkdir(const char *path, u32 mode);
error_t ext2_rmdir(const char *path);
error_t ext2_rename(const char *oldpath, const char *newpath);
error_t ext2_link(const char *oldpath, const char *newpath);
error_t ext2_unlink(const char *path);
error_t ext2_symlink(const char *target, const char *linkpath);
error_t ext2_readlink(const char *path, char *buf, size_t size);
error_t ext2_chmod(const char *path, u32 mode);
error_t ext2_chown(const char *path, u32 uid, u32 gid);
error_t ext2_statfs(const char *path, struct statfs *buf);
file_t *ext2_opendir(const char *path);
error_t ext2_closedir(file_t *dir);
error_t ext2_readdir(file_t *dir, struct dirent *dirent);
error_t ext2_rewinddir(file_t *dir);

/* Ext2 inode operations */
struct inode *ext2_alloc_inode(struct super_block *sb);
void ext2_destroy_inode(struct super_block *sb, struct inode *inode);
error_t ext2_read_inode(struct super_block *sb, struct inode *inode);
error_t ext2_write_inode(struct super_block *sb, struct inode *inode);
error_t ext2_create(struct inode *dir, const char *name, u32 mode, struct inode **inode);
struct inode *ext2_lookup(struct inode *dir, const char *name);
error_t ext2_link(struct inode *inode, struct inode *dir, const char *name);
error_t ext2_unlink(struct inode *dir, const char *name);
error_t ext2_symlink(struct inode *dir, const char *name, const char *target);
error_t ext2_mkdir(struct inode *dir, const char *name, u32 mode);
error_t ext2_rmdir(struct inode *dir, const char *name);
error_t ext2_rename(struct inode *old_dir, const char *old_name, struct inode *new_dir, const char *new_name);
error_t ext2_readlink(struct inode *inode, char *buffer, size_t size);
error_t ext2_follow_link(struct inode *inode, struct inode **target);
error_t ext2_truncate(struct inode *inode, u64 size);
error_t ext2_permission(struct inode *inode, int mask);
error_t ext2_setattr(struct inode *inode, struct iattr *attr);
error_t ext2_getattr(struct inode *inode, struct iattr *attr);

/* Ext2 file operations */
error_t ext2_open_file(file_t *file, u32 flags);
ssize_t ext2_read(file_t *file, void *buffer, size_t size);
ssize_t ext2_write(file_t *file, const void *buffer, size_t size);
error_t ext2_seek(file_t *file, u64 offset, int whence);
loff_t ext2_llseek(file_t *file, loff_t offset, int whence);
error_t ext2_flush(file_t *file);
error_t ext2_fsync(file_t *file);
error_t ext2_close(file_t *file);
int ext2_ioctl(file_t *file, unsigned int cmd, unsigned long arg);
int ext2_mmap(file_t *file, struct vm_area_struct *vma);
int ext2_poll(file_t *file, struct poll_table *wait);
ssize_t ext2_readdir_file(file_t *file, void *dirent, size_t count);

/* Ext2 directory operations */
error_t ext2_open_dir(file_t *file, u32 flags);
error_t ext2_close_dir(file_t *file);
ssize_t ext2_read_dir(file_t *file, void *buffer, size_t size);
error_t ext2_readdir_dir(file_t *file, struct dirent *dirent);

/* Ext2 superblock operations */
error_t ext2_put_super(struct super_block *sb);
error_t ext2_write_super(struct super_block *sb);
error_t ext2_statfs(struct super_block *sb, struct statfs *buf);
error_t ext2_remount_fs(struct super_block *sb, int *flags);
struct super_block *ext2_get_super(const char *dev, u32 flags);
struct super_block *ext2_get_super_from_inode(struct inode *inode);

/* Ext2 utility functions */
u32 ext2_get_block(struct inode *inode, u32 block);
u32 ext2_alloc_block(struct inode *inode, u32 block);
u32 ext2_new_block(struct inode *inode);
int ext2_free_block(struct inode *inode, u32 block);
u32 ext2_new_inode(struct inode *dir);
int ext2_free_inode(struct inode *dir, u32 ino);
error_t ext2_add_entry(struct inode *dir, const char *name, u32 ino, file_type_t type);
error_t ext2_remove_entry(struct inode *dir, const char *name);
int ext2_is_dir_empty(struct inode *dir);
int ext2_read_block(ext2_sb_info_t *sb, u32 block, void *buffer);
int ext2_write_block(ext2_sb_info_t *sb, u32 block, void *buffer);

#endif /* _HORIZON_FS_EXT2_H */
