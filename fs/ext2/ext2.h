/**
 * ext2.h - Ext2 file system definitions
 * 
 * This file contains definitions for the Ext2 file system.
 */

#ifndef _FS_EXT2_H
#define _FS_EXT2_H

#include <horizon/types.h>
#include <horizon/fs.h>

/* Ext2 superblock structure */
typedef struct ext2_superblock {
    u32 inodes_count;           /* Inodes count */
    u32 blocks_count;           /* Blocks count */
    u32 r_blocks_count;         /* Reserved blocks count */
    u32 free_blocks_count;      /* Free blocks count */
    u32 free_inodes_count;      /* Free inodes count */
    u32 first_data_block;       /* First data block */
    u32 log_block_size;         /* Block size */
    u32 log_frag_size;          /* Fragment size */
    u32 blocks_per_group;       /* Blocks per group */
    u32 frags_per_group;        /* Fragments per group */
    u32 inodes_per_group;       /* Inodes per group */
    u32 mtime;                  /* Mount time */
    u32 wtime;                  /* Write time */
    u16 mnt_count;              /* Mount count */
    u16 max_mnt_count;          /* Maximal mount count */
    u16 magic;                  /* Magic signature */
    u16 state;                  /* File system state */
    u16 errors;                 /* Behaviour when detecting errors */
    u16 minor_rev_level;        /* Minor revision level */
    u32 lastcheck;              /* Time of last check */
    u32 checkinterval;          /* Max. time between checks */
    u32 creator_os;             /* OS */
    u32 rev_level;              /* Revision level */
    u16 def_resuid;             /* Default uid for reserved blocks */
    u16 def_resgid;             /* Default gid for reserved blocks */
    /* EXT2_DYNAMIC_REV specific */
    u32 first_ino;              /* First non-reserved inode */
    u16 inode_size;             /* Size of inode structure */
    u16 block_group_nr;         /* Block group # of this superblock */
    u32 feature_compat;         /* Compatible feature set */
    u32 feature_incompat;       /* Incompatible feature set */
    u32 feature_ro_compat;      /* Readonly-compatible feature set */
    u8 uuid[16];                /* 128-bit uuid for volume */
    char volume_name[16];       /* Volume name */
    char last_mounted[64];      /* Directory where last mounted */
    u32 algorithm_usage_bitmap; /* For compression */
    /* Performance hints */
    u8 prealloc_blocks;         /* # of blocks to try to preallocate */
    u8 prealloc_dir_blocks;     /* # of blocks to preallocate for dirs */
    u16 padding1;               /* Padding */
    /* Journaling support */
    u8 journal_uuid[16];        /* UUID of journal superblock */
    u32 journal_inum;           /* Inode number of journal file */
    u32 journal_dev;            /* Device number of journal file */
    u32 last_orphan;            /* Start of list of inodes to delete */
    u32 hash_seed[4];           /* HTREE hash seed */
    u8 def_hash_version;        /* Default hash version to use */
    u8 padding2[3];             /* Padding */
    u32 default_mount_opts;     /* Default mount options */
    u32 first_meta_bg;          /* First metablock block group */
    u32 reserved[190];          /* Padding to the end of the block */
} ext2_superblock_t;

/* Ext2 group descriptor structure */
typedef struct ext2_group_desc {
    u32 block_bitmap;           /* Blocks bitmap block */
    u32 inode_bitmap;           /* Inodes bitmap block */
    u32 inode_table;            /* Inodes table block */
    u16 free_blocks_count;      /* Free blocks count */
    u16 free_inodes_count;      /* Free inodes count */
    u16 used_dirs_count;        /* Directories count */
    u16 pad;                    /* Padding */
    u32 reserved[3];            /* Reserved */
} ext2_group_desc_t;

/* Ext2 inode structure */
typedef struct ext2_inode {
    u16 mode;                   /* File mode */
    u16 uid;                    /* Low 16 bits of Owner Uid */
    u32 size;                   /* Size in bytes */
    u32 atime;                  /* Access time */
    u32 ctime;                  /* Creation time */
    u32 mtime;                  /* Modification time */
    u32 dtime;                  /* Deletion Time */
    u16 gid;                    /* Low 16 bits of Group Id */
    u16 links_count;            /* Links count */
    u32 blocks;                 /* Blocks count */
    u32 flags;                  /* File flags */
    u32 osd1;                   /* OS dependent 1 */
    u32 block[15];              /* Pointers to blocks */
    u32 generation;             /* File version (for NFS) */
    u32 file_acl;               /* File ACL */
    u32 dir_acl;                /* Directory ACL */
    u32 faddr;                  /* Fragment address */
    u8 osd2[12];                /* OS dependent 2 */
} ext2_inode_t;

/* Ext2 directory entry structure */
typedef struct ext2_dir_entry {
    u32 inode;                  /* Inode number */
    u16 rec_len;                /* Directory entry length */
    u8 name_len;                /* Name length */
    u8 file_type;               /* File type */
    char name[];                /* File name */
} ext2_dir_entry_t;

/* Ext2 file system structure */
typedef struct ext2_fs {
    ext2_superblock_t *sb;      /* Superblock */
    ext2_group_desc_t *gd;      /* Group descriptors */
    u32 block_size;             /* Block size */
    u32 groups_count;           /* Number of groups */
    u32 inodes_per_block;       /* Number of inodes per block */
    u32 addr_per_block;         /* Number of addresses per block */
} ext2_fs_t;

/* Ext2 file system operations */
extern file_operations_t ext2_file_ops;

/* Ext2 file system functions */
int ext2_init(void);
int ext2_mount(const char *dev, const char *dir, u32 flags);
int ext2_unmount(const char *dir);

#endif /* _FS_EXT2_H */
