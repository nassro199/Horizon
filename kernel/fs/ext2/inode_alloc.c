/**
 * inode_alloc.c - Ext2 inode allocation operations
 *
 * This file contains the implementation of Ext2 inode allocation operations.
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
 * Allocate a new inode
 * 
 * @param dir Directory inode
 * @return Inode number, or 0 on failure
 */
u32 ext2_new_inode(struct inode *dir) {
    /* Check if the directory exists */
    if (dir == NULL) {
        printk(KERN_ERR "EXT2: Directory does not exist\n");
        return 0;
    }
    
    /* Get the superblock */
    super_block_t *sb = dir->i_ops->get_super(dir);
    
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Calculate the block size */
    u32 block_size = sbi->s_block_size;
    
    /* Check if there are any free inodes */
    if (sbi->s_es->s_free_inodes_count == 0) {
        printk(KERN_ERR "EXT2: No free inodes\n");
        return 0;
    }
    
    /* Get the Ext2 inode info */
    ext2_inode_info_t *ei = (ext2_inode_info_t *)dir->fs_data;
    
    /* Try to allocate an inode in the same block group as the directory */
    u32 group = ei->i_block_group;
    
    /* Check if there are any free inodes in this group */
    if (sbi->s_group_desc[group].bg_free_inodes_count == 0) {
        /* Try to find a group with free inodes */
        for (u32 i = 0; i < sbi->s_groups_count; i++) {
            if (sbi->s_group_desc[i].bg_free_inodes_count > 0) {
                group = i;
                break;
            }
        }
        
        /* Check if we found a group */
        if (sbi->s_group_desc[group].bg_free_inodes_count == 0) {
            printk(KERN_ERR "EXT2: No free inodes\n");
            return 0;
        }
    }
    
    /* Allocate memory for the inode bitmap */
    u8 *bitmap = kmalloc(block_size, 0);
    
    if (bitmap == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for inode bitmap\n");
        return 0;
    }
    
    /* Read the inode bitmap */
    int ret = ext2_read_block(sbi, sbi->s_group_desc[group].bg_inode_bitmap, bitmap);
    
    if (ret < 0) {
        kfree(bitmap);
        return 0;
    }
    
    /* Find a free inode */
    for (u32 i = 0; i < sbi->s_inodes_per_group; i++) {
        /* Check if the inode is free */
        if (!(bitmap[i / 8] & (1 << (i % 8)))) {
            /* Mark the inode as used */
            bitmap[i / 8] |= (1 << (i % 8));
            
            /* Write the inode bitmap */
            ret = ext2_write_block(sbi, sbi->s_group_desc[group].bg_inode_bitmap, bitmap);
            
            if (ret < 0) {
                kfree(bitmap);
                return 0;
            }
            
            /* Free the bitmap */
            kfree(bitmap);
            
            /* Update the group descriptor */
            sbi->s_group_desc[group].bg_free_inodes_count--;
            
            /* Update the superblock */
            sbi->s_es->s_free_inodes_count--;
            
            /* Write the superblock */
            ret = ext2_write_super(sb);
            
            if (ret < 0) {
                return 0;
            }
            
            /* Calculate the inode number */
            u32 ino = group * sbi->s_inodes_per_group + i + 1;
            
            return ino;
        }
    }
    
    /* No free inodes found */
    kfree(bitmap);
    
    return 0;
}

/**
 * Free an inode
 * 
 * @param dir Directory inode
 * @param ino Inode number to free
 * @return 0 on success, negative error code on failure
 */
int ext2_free_inode(struct inode *dir, u32 ino) {
    /* Check if the directory exists */
    if (dir == NULL) {
        printk(KERN_ERR "EXT2: Directory does not exist\n");
        return -ENOENT;
    }
    
    /* Get the superblock */
    super_block_t *sb = dir->i_ops->get_super(dir);
    
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Calculate the block size */
    u32 block_size = sbi->s_block_size;
    
    /* Calculate the block group */
    u32 group = (ino - 1) / sbi->s_inodes_per_group;
    
    /* Calculate the index within the block group */
    u32 index = (ino - 1) % sbi->s_inodes_per_group;
    
    /* Allocate memory for the inode bitmap */
    u8 *bitmap = kmalloc(block_size, 0);
    
    if (bitmap == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for inode bitmap\n");
        return -ENOMEM;
    }
    
    /* Read the inode bitmap */
    int ret = ext2_read_block(sbi, sbi->s_group_desc[group].bg_inode_bitmap, bitmap);
    
    if (ret < 0) {
        kfree(bitmap);
        return ret;
    }
    
    /* Check if the inode is already free */
    if (!(bitmap[index / 8] & (1 << (index % 8)))) {
        kfree(bitmap);
        return 0;
    }
    
    /* Mark the inode as free */
    bitmap[index / 8] &= ~(1 << (index % 8));
    
    /* Write the inode bitmap */
    ret = ext2_write_block(sbi, sbi->s_group_desc[group].bg_inode_bitmap, bitmap);
    
    if (ret < 0) {
        kfree(bitmap);
        return ret;
    }
    
    /* Free the bitmap */
    kfree(bitmap);
    
    /* Update the group descriptor */
    sbi->s_group_desc[group].bg_free_inodes_count++;
    
    /* Update the superblock */
    sbi->s_es->s_free_inodes_count++;
    
    /* Write the superblock */
    ret = ext2_write_super(sb);
    
    return ret;
}

/**
 * Link an inode to a directory
 * 
 * @param inode Inode to link
 * @param dir Directory to link to
 * @param name Name of the link
 * @return 0 on success, negative error code on failure
 */
error_t ext2_link(struct inode *inode, struct inode *dir, const char *name) {
    /* Check if the inode exists */
    if (inode == NULL) {
        printk(KERN_ERR "EXT2: Inode does not exist\n");
        return -ENOENT;
    }
    
    /* Check if the directory exists */
    if (dir == NULL) {
        printk(KERN_ERR "EXT2: Directory does not exist\n");
        return -ENOENT;
    }
    
    /* Check if the directory is a directory */
    if (dir->type != FILE_TYPE_DIRECTORY) {
        printk(KERN_ERR "EXT2: Not a directory\n");
        return -ENOTDIR;
    }
    
    /* Check if the name is valid */
    if (name == NULL || name[0] == '\0') {
        printk(KERN_ERR "EXT2: Invalid name\n");
        return -EINVAL;
    }
    
    /* Check if the file already exists */
    struct inode *existing = ext2_lookup(dir, name);
    
    if (existing != NULL) {
        /* File already exists */
        ext2_destroy_inode(NULL, existing);
        return -EEXIST;
    }
    
    /* Add the entry to the directory */
    error_t ret = ext2_add_entry(dir, name, inode->inode_num, inode->type);
    
    if (ret < 0) {
        return ret;
    }
    
    /* Increment the link count */
    inode->links++;
    
    /* Write the inode */
    ret = ext2_write_inode(dir->i_ops->get_super(dir), inode);
    
    return ret;
}

/**
 * Unlink an inode from a directory
 * 
 * @param dir Directory to unlink from
 * @param name Name of the link to remove
 * @return 0 on success, negative error code on failure
 */
error_t ext2_unlink(struct inode *dir, const char *name) {
    /* Check if the directory exists */
    if (dir == NULL) {
        printk(KERN_ERR "EXT2: Directory does not exist\n");
        return -ENOENT;
    }
    
    /* Check if the directory is a directory */
    if (dir->type != FILE_TYPE_DIRECTORY) {
        printk(KERN_ERR "EXT2: Not a directory\n");
        return -ENOTDIR;
    }
    
    /* Check if the name is valid */
    if (name == NULL || name[0] == '\0') {
        printk(KERN_ERR "EXT2: Invalid name\n");
        return -EINVAL;
    }
    
    /* Look up the inode */
    struct inode *inode = ext2_lookup(dir, name);
    
    if (inode == NULL) {
        return -ENOENT;
    }
    
    /* Remove the entry from the directory */
    error_t ret = ext2_remove_entry(dir, name);
    
    if (ret < 0) {
        ext2_destroy_inode(NULL, inode);
        return ret;
    }
    
    /* Decrement the link count */
    inode->links--;
    
    /* Check if the inode should be deleted */
    if (inode->links == 0) {
        /* Free the inode */
        ret = ext2_free_inode(dir, inode->inode_num);
    } else {
        /* Write the inode */
        ret = ext2_write_inode(dir->i_ops->get_super(dir), inode);
    }
    
    /* Free the inode */
    ext2_destroy_inode(NULL, inode);
    
    return ret;
}
