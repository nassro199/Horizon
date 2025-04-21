/**
 * super.c - Ext2 superblock operations
 *
 * This file contains the implementation of Ext2 superblock operations.
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

/**
 * Put the superblock
 * 
 * @param sb Superblock
 * @return 0 on success, negative error code on failure
 */
error_t ext2_put_super(struct super_block *sb) {
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Free the resources */
    if (sbi != NULL) {
        if (sbi->s_group_desc != NULL) {
            kfree(sbi->s_group_desc);
        }
        
        if (sbi->s_es != NULL) {
            kfree(sbi->s_es);
        }
        
        kfree(sbi);
    }
    
    /* Clear the superblock */
    sb->fs_data = NULL;
    
    return 0;
}

/**
 * Write the superblock to the device
 * 
 * @param sb Superblock
 * @return 0 on success, negative error code on failure
 */
error_t ext2_write_super(struct super_block *sb) {
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Write the superblock */
    ssize_t ret = device_write(sbi->s_blockdev, sbi->s_es, sizeof(ext2_superblock_t), 1024);
    
    if (ret != sizeof(ext2_superblock_t)) {
        printk(KERN_ERR "EXT2: Failed to write superblock\n");
        return -EIO;
    }
    
    /* Write the group descriptors */
    u32 gdesc_blocks = (sbi->s_groups_count + sbi->s_desc_per_block - 1) / sbi->s_desc_per_block;
    u32 gdesc_block = sbi->s_first_data_block + 1;
    
    for (u32 i = 0; i < gdesc_blocks; i++) {
        ret = ext2_write_block(sbi, gdesc_block + i, (u8 *)sbi->s_group_desc + i * sbi->s_block_size);
        
        if (ret < 0) {
            printk(KERN_ERR "EXT2: Failed to write group descriptors\n");
            return ret;
        }
    }
    
    /* Update the VFS superblock */
    sb->free_blocks = sbi->s_es->s_free_blocks_count;
    sb->free_inodes = sbi->s_es->s_free_inodes_count;
    
    return 0;
}

/**
 * Get file system statistics
 * 
 * @param sb Superblock
 * @param buf Buffer to fill
 * @return 0 on success, negative error code on failure
 */
error_t ext2_statfs(struct super_block *sb, struct statfs *buf) {
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Fill the buffer */
    buf->type = EXT2_MAGIC;
    buf->block_size = sbi->s_block_size;
    buf->blocks = sbi->s_es->s_blocks_count;
    buf->blocks_free = sbi->s_es->s_free_blocks_count;
    buf->blocks_avail = sbi->s_es->s_free_blocks_count;
    buf->files = sbi->s_es->s_inodes_count;
    buf->files_free = sbi->s_es->s_free_inodes_count;
    buf->namelen = 255;
    
    return 0;
}

/**
 * Remount the file system
 * 
 * @param sb Superblock
 * @param flags New flags
 * @return 0 on success, negative error code on failure
 */
error_t ext2_remount_fs(struct super_block *sb, int *flags) {
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Check if the file system is clean */
    if (sbi->s_es->s_state != 1) {
        printk(KERN_WARNING "EXT2: File system is not clean, mounting read-only\n");
        *flags |= MOUNT_READ_ONLY;
    }
    
    /* Update the superblock flags */
    sb->flags = *flags;
    
    return 0;
}

/**
 * Get the superblock for a device
 * 
 * @param dev Device to get superblock for
 * @param flags Mount flags
 * @return Pointer to the superblock, or NULL on failure
 */
struct super_block *ext2_get_super(const char *dev, u32 flags) {
    /* Open the device */
    void *blockdev = device_open(dev, 0);
    
    if (blockdev == NULL) {
        printk(KERN_ERR "EXT2: Failed to open device %s\n", dev);
        return NULL;
    }
    
    /* Allocate memory for the superblock info */
    ext2_sb_info_t *sb = kmalloc(sizeof(ext2_sb_info_t), 0);
    
    if (sb == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for superblock info\n");
        device_close(blockdev);
        return NULL;
    }
    
    /* Initialize the superblock info */
    memset(sb, 0, sizeof(ext2_sb_info_t));
    sb->s_blockdev = blockdev;
    
    /* Read the superblock */
    int ret = ext2_read_super(sb);
    
    if (ret < 0) {
        device_close(blockdev);
        kfree(sb);
        return NULL;
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
        return NULL;
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
            return NULL;
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
        return NULL;
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
    
    return super;
}
