/**
 * btrfs.h - Horizon kernel BTRFS file system definitions
 * 
 * This file contains definitions for the BTRFS file system.
 */

#ifndef _KERNEL_FS_BTRFS_H
#define _KERNEL_FS_BTRFS_H

#include <horizon/types.h>
#include <horizon/fs/vfs.h>

/* BTRFS magic number */
#define BTRFS_MAGIC 0x4D5F53665248425F /* _BHRfS_M */

/* BTRFS superblock */
struct btrfs_super_block {
    u8 csum[32];                /* Checksum of the superblock */
    u8 fsid[16];                /* FS UUID */
    u64 bytenr;                 /* Location of this superblock */
    u64 flags;                  /* Superblock flags */
    u64 magic;                  /* Magic number */
    u64 generation;             /* Generation */
    u64 root;                   /* Root tree root */
    u64 chunk_root;             /* Chunk tree root */
    u64 log_root;               /* Log tree root */
    u64 log_root_transid;       /* Log tree transid */
    u64 total_bytes;            /* Total bytes */
    u64 bytes_used;             /* Bytes used */
    u64 root_dir_objectid;      /* Root directory objectid */
    u64 num_devices;            /* Number of devices */
    u32 sectorsize;             /* Sector size */
    u32 nodesize;               /* Node size */
    u32 leafsize;               /* Leaf size */
    u32 stripesize;             /* Stripe size */
    u32 sys_chunk_array_size;   /* System chunk array size */
    u64 chunk_root_generation;  /* Chunk root generation */
    u64 compat_flags;           /* Compatible flags */
    u64 compat_ro_flags;        /* Compatible read-only flags */
    u64 incompat_flags;         /* Incompatible flags */
    u16 csum_type;              /* Checksum type */
    u8 root_level;              /* Root level */
    u8 chunk_root_level;        /* Chunk root level */
    u8 log_root_level;          /* Log root level */
    struct btrfs_dev_item dev_item; /* Device item */
    char label[256];            /* Label */
    u64 cache_generation;       /* Cache generation */
    u64 uuid_tree_generation;   /* UUID tree generation */
    u64 reserved[30];           /* Reserved */
    u8 sys_chunk_array[2048];   /* System chunk array */
    struct btrfs_root_backup backup_roots[4]; /* Backup roots */
};

/* BTRFS device item */
struct btrfs_dev_item {
    u64 devid;                  /* Device ID */
    u64 total_bytes;            /* Total bytes */
    u64 bytes_used;             /* Bytes used */
    u32 io_align;               /* I/O alignment */
    u32 io_width;               /* I/O width */
    u32 sector_size;            /* Sector size */
    u64 type;                   /* Type */
    u64 generation;             /* Generation */
    u64 start_offset;           /* Start offset */
    u32 dev_group;              /* Device group */
    u8 seek_speed;              /* Seek speed */
    u8 bandwidth;               /* Bandwidth */
    u8 uuid[16];                /* UUID */
    u8 fsid[16];                /* FS UUID */
};

/* BTRFS root backup */
struct btrfs_root_backup {
    u64 tree_root;              /* Tree root */
    u64 tree_root_gen;          /* Tree root generation */
    u64 chunk_root;             /* Chunk root */
    u64 chunk_root_gen;         /* Chunk root generation */
    u64 extent_root;            /* Extent root */
    u64 extent_root_gen;        /* Extent root generation */
    u64 fs_root;                /* FS root */
    u64 fs_root_gen;            /* FS root generation */
    u64 dev_root;               /* Device root */
    u64 dev_root_gen;           /* Device root generation */
    u64 csum_root;              /* Checksum root */
    u64 csum_root_gen;          /* Checksum root generation */
    u64 total_bytes;            /* Total bytes */
    u64 bytes_used;             /* Bytes used */
    u64 num_devices;            /* Number of devices */
    u64 reserved[4];            /* Reserved */
};

/* BTRFS functions */
extern struct file_system_type btrfs_fs_type;
extern struct super_operations btrfs_super_ops;
extern struct inode_operations btrfs_dir_inode_ops;
extern struct inode_operations btrfs_file_inode_ops;
extern struct file_operations btrfs_dir_ops;
extern struct file_operations btrfs_file_ops;

/* BTRFS initialization */
int btrfs_init(void);

#endif /* _KERNEL_FS_BTRFS_H */
