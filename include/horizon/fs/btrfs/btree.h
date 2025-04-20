/**
 * btree.h - Horizon kernel BTRFS B-tree definitions
 * 
 * This file contains definitions for the BTRFS B-tree operations.
 */

#ifndef _KERNEL_FS_BTRFS_BTREE_H
#define _KERNEL_FS_BTRFS_BTREE_H

#include <horizon/types.h>
#include <horizon/fs/btrfs/disk_format.h>

/* BTRFS path */
struct btrfs_path {
    struct btrfs_node *nodes[BTRFS_MAX_LEVEL]; /* Nodes */
    u32 slots[BTRFS_MAX_LEVEL];   /* Slots */
    u8 reada;                     /* Read ahead */
    u8 lowest_level;              /* Lowest level */
};

/* BTRFS key */
struct btrfs_key {
    u64 objectid;                 /* Object ID */
    u8 type;                      /* Type */
    u64 offset;                   /* Offset */
};

/* BTRFS root */
struct btrfs_root {
    struct btrfs_fs_info *fs_info; /* FS info */
    struct btrfs_key root_key;    /* Root key */
    struct btrfs_root_item *root_item; /* Root item */
    struct btrfs_node *node;      /* Node */
    u32 slot;                     /* Slot */
    u64 commit_root;              /* Commit root */
    u64 last_trans;               /* Last transaction */
    u32 refs;                     /* References */
    u64 last_inode_alloc;         /* Last inode allocation */
    struct list_head dirty_list;  /* Dirty list */
    struct list_head root_list;   /* Root list */
};

/* BTRFS FS info */
struct btrfs_fs_info {
    struct btrfs_super_block *super_copy; /* Super block copy */
    struct btrfs_root *tree_root;  /* Tree root */
    struct btrfs_root *chunk_root; /* Chunk root */
    struct btrfs_root *dev_root;   /* Device root */
    struct btrfs_root *fs_root;    /* FS root */
    struct btrfs_root *csum_root;  /* Checksum root */
    struct btrfs_root *quota_root; /* Quota root */
    struct btrfs_root *uuid_root;  /* UUID root */
    struct btrfs_root *free_space_root; /* Free space root */
    struct list_head fs_root_list; /* FS root list */
    u64 tree_root_bytenr;         /* Tree root block number */
    u64 chunk_root_bytenr;        /* Chunk root block number */
    u64 dev_root_bytenr;          /* Device root block number */
    u64 fs_root_bytenr;           /* FS root block number */
    u64 csum_root_bytenr;         /* Checksum root block number */
    u64 quota_root_bytenr;        /* Quota root block number */
    u64 uuid_root_bytenr;         /* UUID root block number */
    u64 free_space_root_bytenr;   /* Free space root block number */
    u64 generation;               /* Generation */
    u64 last_trans_committed;     /* Last transaction committed */
    u32 sectorsize;               /* Sector size */
    u32 nodesize;                 /* Node size */
    u32 leafsize;                 /* Leaf size */
    u32 stripesize;               /* Stripe size */
    u32 csum_size;                /* Checksum size */
    u16 csum_type;                /* Checksum type */
    u64 total_bytes;              /* Total bytes */
    u64 bytes_used;               /* Bytes used */
    u64 num_devices;              /* Number of devices */
    u64 flags;                    /* Flags */
    u64 cache_generation;         /* Cache generation */
    u64 uuid_tree_generation;     /* UUID tree generation */
};

/* BTRFS B-tree functions */
int btrfs_search_slot(struct btrfs_root *root, struct btrfs_key *key, struct btrfs_path *path, int ins_len, int cow);
int btrfs_insert_item(struct btrfs_root *root, struct btrfs_key *key, void *data, u32 data_size);
int btrfs_delete_item(struct btrfs_root *root, struct btrfs_key *key);
int btrfs_update_item(struct btrfs_root *root, struct btrfs_key *key, void *data, u32 data_size);
int btrfs_lookup_item(struct btrfs_root *root, struct btrfs_key *key, void *data, u32 *data_size);
int btrfs_next_item(struct btrfs_root *root, struct btrfs_key *key);
int btrfs_prev_item(struct btrfs_root *root, struct btrfs_key *key);
int btrfs_first_item(struct btrfs_root *root, struct btrfs_key *key);
int btrfs_last_item(struct btrfs_root *root, struct btrfs_key *key);

#endif /* _KERNEL_FS_BTRFS_BTREE_H */
