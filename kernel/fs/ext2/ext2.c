/**
 * ext2.c - Ext2 file system implementation
 *
 * This file contains the implementation of the Ext2 file system.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/fs.h>
#include <horizon/fs/ext2.h>
#include <horizon/mm.h>
#include <horizon/device.h>
#include <horizon/printk.h>
#include <horizon/errno.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Ext2 file system type */
static file_system_type_t ext2_fs_type = {
    .name = "ext2",
    .flags = 0,
    .mount = ext2_mount,
    .unmount = ext2_unmount
};

/* Ext2 superblock operations */
static super_operations_t ext2_super_ops = {
    .alloc_inode = ext2_alloc_inode,
    .destroy_inode = ext2_destroy_inode,
    .write_inode = ext2_write_inode,
    .read_inode = ext2_read_inode,
    .put_super = ext2_put_super,
    .write_super = ext2_write_super,
    .statfs = ext2_statfs,
    .remount_fs = ext2_remount_fs
};

/* Ext2 inode operations */
static inode_operations_t ext2_inode_ops = {
    .lookup = ext2_lookup,
    .create = ext2_create,
    .link = ext2_link,
    .unlink = ext2_unlink,
    .symlink = ext2_symlink,
    .mkdir = ext2_mkdir,
    .rmdir = ext2_rmdir,
    .rename = ext2_rename,
    .readlink = ext2_readlink,
    .follow_link = ext2_follow_link,
    .truncate = ext2_truncate,
    .permission = ext2_permission,
    .setattr = ext2_setattr,
    .getattr = ext2_getattr
};

/* Ext2 file operations */
static file_operations_t ext2_file_ops = {
    .read = ext2_read,
    .write = ext2_write,
    .open = ext2_open_file,
    .close = ext2_close,
    .seek = ext2_seek,
    .flush = ext2_flush,
    .fsync = ext2_fsync,
    .ioctl = ext2_ioctl,
    .mmap = ext2_mmap,
    .readdir = ext2_readdir_file
};

/* Ext2 directory operations */
static file_operations_t ext2_dir_ops = {
    .read = ext2_read_dir,
    .open = ext2_open_dir,
    .close = ext2_close_dir,
    .readdir = ext2_readdir_dir
};

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

/**
 * Initialize the Ext2 file system
 *
 * @return 0 on success, negative error code on failure
 */
int ext2_init(void) {
    /* Register the file system */
    int ret = fs_register("ext2", ext2_mount, ext2_unmount);

    if (ret < 0) {
        printk(KERN_ERR "EXT2: Failed to register file system\n");
        return ret;
    }

    printk(KERN_INFO "EXT2: Initialized Ext2 file system\n");

    return 0;
}

/**
 * Read a block from the device
 *
 * @param sb Superblock
 * @param block Block number
 * @param buffer Buffer to read into
 * @return 0 on success, negative error code on failure
 */
static int ext2_read_block(ext2_sb_info_t *sb, u32 block, void *buffer) {
    /* Calculate the offset */
    u64 offset = block * sb->s_block_size;

    /* Read the block */
    ssize_t ret = device_read(sb->s_blockdev, buffer, sb->s_block_size, offset);

    if (ret != sb->s_block_size) {
        printk(KERN_ERR "EXT2: Failed to read block %u\n", block);
        return -EIO;
    }

    return 0;
}

/**
 * Write a block to the device
 *
 * @param sb Superblock
 * @param block Block number
 * @param buffer Buffer to write from
 * @return 0 on success, negative error code on failure
 */
static int ext2_write_block(ext2_sb_info_t *sb, u32 block, void *buffer) {
    /* Calculate the offset */
    u64 offset = block * sb->s_block_size;

    /* Write the block */
    ssize_t ret = device_write(sb->s_blockdev, buffer, sb->s_block_size, offset);

    if (ret != sb->s_block_size) {
        printk(KERN_ERR "EXT2: Failed to write block %u\n", block);
        return -EIO;
    }

    return 0;
}

/**
 * Read the superblock from the device
 *
 * @param sb Superblock info
 * @return 0 on success, negative error code on failure
 */
static int ext2_read_super(ext2_sb_info_t *sb) {
    /* Allocate memory for the superblock */
    sb->s_es = kmalloc(sizeof(ext2_superblock_t), 0);

    if (sb->s_es == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for superblock\n");
        return -ENOMEM;
    }

    /* Read the superblock */
    ssize_t ret = device_read(sb->s_blockdev, sb->s_es, sizeof(ext2_superblock_t), 1024);

    if (ret != sizeof(ext2_superblock_t)) {
        printk(KERN_ERR "EXT2: Failed to read superblock\n");
        kfree(sb->s_es);
        return -EIO;
    }

    /* Check the magic number */
    if (sb->s_es->s_magic != EXT2_MAGIC) {
        printk(KERN_ERR "EXT2: Invalid magic number: 0x%04x\n", sb->s_es->s_magic);
        kfree(sb->s_es);
        return -EINVAL;
    }

    return 0;
}

/**
 * Mount an Ext2 file system
 *
 * @param dev Device to mount
 * @param dir Directory to mount on
 * @param flags Mount flags
 * @return 0 on success, negative error code on failure
 */
int ext2_mount(const char *dev, const char *dir, u32 flags) {
    /* Open the device */
    void *blockdev = device_open(dev, 0);

    if (blockdev == NULL) {
        printk(KERN_ERR "EXT2: Failed to open device %s\n", dev);
        return -ENODEV;
    }

    /* Allocate memory for the superblock info */
    ext2_sb_info_t *sb = kmalloc(sizeof(ext2_sb_info_t), 0);

    if (sb == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for superblock info\n");
        device_close(blockdev);
        return -ENOMEM;
    }

    /* Initialize the superblock info */
    memset(sb, 0, sizeof(ext2_sb_info_t));
    sb->s_blockdev = blockdev;

    /* Read the superblock */
    int ret = ext2_read_super(sb);

    if (ret < 0) {
        device_close(blockdev);
        kfree(sb);
        return ret;
    }

    /* Calculate file system parameters */
    sb->s_block_size = 1024 << sb->s_es->s_log_block_size;
    sb->s_inodes_per_block = sb->s_block_size / sb->s_es->s_inode_size;
    sb->s_blocks_per_group = sb->s_es->s_blocks_per_group;
    sb->s_inodes_per_group = sb->s_es->s_inodes_per_group;
    sb->s_itb_per_group = sb->s_inodes_per_group / sb->s_inodes_per_block;
    sb->s_desc_per_block = sb->s_block_size / sizeof(ext2_group_desc_t);
    sb->s_groups_count = (sb->s_es->s_blocks_count - sb->s_es->s_first_data_block + sb->s_blocks_per_group - 1) / sb->s_blocks_per_group;
    sb->s_first_data_block = sb->s_es->s_first_data_block;
    sb->s_first_ino = sb->s_es->s_first_ino;
    sb->s_inode_size = sb->s_es->s_inode_size;

    /* Read the group descriptors */
    u32 gdesc_blocks = (sb->s_groups_count + sb->s_desc_per_block - 1) / sb->s_desc_per_block;
    u32 gdesc_size = gdesc_blocks * sb->s_block_size;

    sb->s_group_desc = kmalloc(gdesc_size, 0);

    if (sb->s_group_desc == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for group descriptors\n");
        kfree(sb->s_es);
        device_close(blockdev);
        kfree(sb);
        return -ENOMEM;
    }

    /* Read the group descriptors */
    u32 gdesc_block = sb->s_first_data_block + 1;

    for (u32 i = 0; i < gdesc_blocks; i++) {
        ret = ext2_read_block(sb, gdesc_block + i, (u8 *)sb->s_group_desc + i * sb->s_block_size);

        if (ret < 0) {
            kfree(sb->s_group_desc);
            kfree(sb->s_es);
            device_close(blockdev);
            kfree(sb);
            return ret;
        }
    }

    /* Create a superblock */
    super_block_t *super = kmalloc(sizeof(super_block_t), 0);

    if (super == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for VFS superblock\n");
        kfree(sb->s_group_desc);
        kfree(sb->s_es);
        device_close(blockdev);
        kfree(sb);
        return -ENOMEM;
    }

    /* Initialize the superblock */
    super->magic = EXT2_MAGIC;
    super->block_size = sb->s_block_size;
    super->total_blocks = sb->s_es->s_blocks_count;
    super->free_blocks = sb->s_es->s_free_blocks_count;
    super->total_inodes = sb->s_es->s_inodes_count;
    super->free_inodes = sb->s_es->s_free_inodes_count;
    super->flags = flags;
    super->fs_data = sb;
    super->s_ops = &ext2_super_ops;

    /* Mount the file system */
    ret = fs_mount_super(dir, super);

    if (ret < 0) {
        printk(KERN_ERR "EXT2: Failed to mount file system\n");
        kfree(super);
        kfree(sb->s_group_desc);
        kfree(sb->s_es);
        device_close(blockdev);
        kfree(sb);
        return ret;
    }

    printk(KERN_INFO "EXT2: Mounted %s on %s\n", dev, dir);

    return 0;
}

/**
 * Unmount an Ext2 file system
 *
 * @param dir Directory to unmount
 * @return 0 on success, negative error code on failure
 */
int ext2_unmount(const char *dir) {
    /* Get the superblock */
    super_block_t *super = fs_get_super(dir);

    if (super == NULL) {
        printk(KERN_ERR "EXT2: Failed to get superblock for %s\n", dir);
        return -EINVAL;
    }

    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sb = (ext2_sb_info_t *)super->fs_data;

    /* Unmount the file system */
    int ret = fs_unmount_super(dir);

    if (ret < 0) {
        printk(KERN_ERR "EXT2: Failed to unmount file system\n");
        return ret;
    }

    /* Free the resources */
    kfree(sb->s_group_desc);
    kfree(sb->s_es);
    device_close(sb->s_blockdev);
    kfree(sb);
    kfree(super);

    printk(KERN_INFO "EXT2: Unmounted %s\n", dir);

    return 0;
}
