/**
 * disk_format.h - Horizon kernel BTRFS disk format definitions
 * 
 * This file contains definitions for the BTRFS disk format.
 */

#ifndef _KERNEL_FS_BTRFS_DISK_FORMAT_H
#define _KERNEL_FS_BTRFS_DISK_FORMAT_H

#include <horizon/types.h>

/* BTRFS object IDs */
#define BTRFS_ROOT_TREE_OBJECTID     1ULL
#define BTRFS_EXTENT_TREE_OBJECTID   2ULL
#define BTRFS_CHUNK_TREE_OBJECTID    3ULL
#define BTRFS_DEV_TREE_OBJECTID      4ULL
#define BTRFS_FS_TREE_OBJECTID       5ULL
#define BTRFS_ROOT_TREE_DIR_OBJECTID 6ULL
#define BTRFS_CSUM_TREE_OBJECTID     7ULL
#define BTRFS_QUOTA_TREE_OBJECTID    8ULL
#define BTRFS_UUID_TREE_OBJECTID     9ULL
#define BTRFS_FREE_SPACE_TREE_OBJECTID 10ULL
#define BTRFS_FIRST_FREE_OBJECTID    256ULL
#define BTRFS_LAST_FREE_OBJECTID     -256ULL
#define BTRFS_FIRST_CHUNK_TREE_OBJECTID 256ULL
#define BTRFS_DEV_ITEMS_OBJECTID     1ULL

/* BTRFS item keys */
#define BTRFS_INODE_ITEM_KEY         1
#define BTRFS_INODE_REF_KEY          12
#define BTRFS_INODE_EXTREF_KEY       13
#define BTRFS_XATTR_ITEM_KEY         24
#define BTRFS_ORPHAN_ITEM_KEY        48
#define BTRFS_DIR_LOG_ITEM_KEY       60
#define BTRFS_DIR_LOG_INDEX_KEY      72
#define BTRFS_DIR_ITEM_KEY           84
#define BTRFS_DIR_INDEX_KEY          96
#define BTRFS_EXTENT_DATA_KEY        108
#define BTRFS_EXTENT_CSUM_KEY        128
#define BTRFS_ROOT_ITEM_KEY          132
#define BTRFS_ROOT_BACKREF_KEY       144
#define BTRFS_ROOT_REF_KEY           156
#define BTRFS_EXTENT_ITEM_KEY        168
#define BTRFS_METADATA_ITEM_KEY      169
#define BTRFS_TREE_BLOCK_REF_KEY     176
#define BTRFS_EXTENT_DATA_REF_KEY    178
#define BTRFS_EXTENT_REF_V0_KEY      180
#define BTRFS_SHARED_BLOCK_REF_KEY   182
#define BTRFS_SHARED_DATA_REF_KEY    184
#define BTRFS_BLOCK_GROUP_ITEM_KEY   192
#define BTRFS_DEV_EXTENT_KEY         204
#define BTRFS_DEV_ITEM_KEY           216
#define BTRFS_CHUNK_ITEM_KEY         228
#define BTRFS_QGROUP_STATUS_KEY      240
#define BTRFS_QGROUP_INFO_KEY        242
#define BTRFS_QGROUP_LIMIT_KEY       244
#define BTRFS_QGROUP_RELATION_KEY    246
#define BTRFS_BALANCE_ITEM_KEY       248
#define BTRFS_TEMPORARY_ITEM_KEY     248
#define BTRFS_DEV_STATS_KEY          249
#define BTRFS_PERSISTENT_ITEM_KEY    249
#define BTRFS_DEV_REPLACE_KEY        250
#define BTRFS_UUID_KEY_SUBVOL        251
#define BTRFS_UUID_KEY_RECEIVED_SUBVOL 252
#define BTRFS_STRING_ITEM_KEY        253

/* BTRFS key */
struct btrfs_disk_key {
    u64 objectid;               /* Object ID */
    u8 type;                    /* Type */
    u64 offset;                 /* Offset */
};

/* BTRFS header */
struct btrfs_header {
    u8 csum[32];                /* Checksum */
    u8 fsid[16];                /* FS UUID */
    u64 bytenr;                 /* Block number */
    u64 flags;                  /* Flags */
    u8 chunk_tree_uuid[16];     /* Chunk tree UUID */
    u64 generation;             /* Generation */
    u64 owner;                  /* Owner */
    u32 nritems;                /* Number of items */
    u8 level;                   /* Level */
};

/* BTRFS item */
struct btrfs_item {
    struct btrfs_disk_key key;  /* Key */
    u32 offset;                 /* Offset */
    u32 size;                   /* Size */
};

/* BTRFS leaf */
struct btrfs_leaf {
    struct btrfs_header header; /* Header */
    struct btrfs_item items[];  /* Items */
};

/* BTRFS node */
struct btrfs_node {
    struct btrfs_header header; /* Header */
    struct btrfs_key_ptr ptrs[]; /* Pointers */
};

/* BTRFS key pointer */
struct btrfs_key_ptr {
    struct btrfs_disk_key key;  /* Key */
    u64 blockptr;               /* Block pointer */
    u64 generation;             /* Generation */
};

/* BTRFS inode item */
struct btrfs_inode_item {
    u64 generation;             /* Generation */
    u64 transid;                /* Transaction ID */
    u64 size;                   /* Size */
    u64 nbytes;                 /* Number of bytes */
    u64 block_group;            /* Block group */
    u32 nlink;                  /* Number of links */
    u32 uid;                    /* User ID */
    u32 gid;                    /* Group ID */
    u32 mode;                   /* Mode */
    u64 rdev;                   /* Device ID */
    u64 flags;                  /* Flags */
    u64 sequence;               /* Sequence */
    u64 reserved[4];            /* Reserved */
    struct btrfs_timespec atime; /* Access time */
    struct btrfs_timespec ctime; /* Change time */
    struct btrfs_timespec mtime; /* Modification time */
    struct btrfs_timespec otime; /* Creation time */
};

/* BTRFS timespec */
struct btrfs_timespec {
    u64 sec;                    /* Seconds */
    u32 nsec;                   /* Nanoseconds */
};

#endif /* _KERNEL_FS_BTRFS_DISK_FORMAT_H */
