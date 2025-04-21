/**
 * symlink.c - Ext2 symbolic link operations
 *
 * This file contains the implementation of Ext2 symbolic link operations.
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
#include <horizon/time.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/**
 * Create a symbolic link
 * 
 * @param dir Parent directory inode
 * @param name Name of the symbolic link
 * @param target Target of the symbolic link
 * @return 0 on success, negative error code on failure
 */
error_t ext2_symlink(struct inode *dir, const char *name, const char *target) {
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
    
    /* Check if the target is valid */
    if (target == NULL || target[0] == '\0') {
        printk(KERN_ERR "EXT2: Invalid target\n");
        return -EINVAL;
    }
    
    /* Get the target length */
    u32 target_len = strlen(target);
    
    /* Check if the target is too long */
    if (target_len > 60) {
        printk(KERN_ERR "EXT2: Target too long\n");
        return -ENAMETOOLONG;
    }
    
    /* Create the symbolic link inode */
    struct inode *inode;
    error_t ret = ext2_create(dir, name, EXT2_S_IFLNK | 0777, &inode);
    
    if (ret < 0) {
        return ret;
    }
    
    /* Get the Ext2 inode info */
    ext2_inode_info_t *ei = (ext2_inode_info_t *)inode->fs_data;
    
    /* Get the superblock */
    super_block_t *sb = dir->i_ops->get_super(dir);
    
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Check if the target fits in the inode */
    if (target_len <= 60) {
        /* Store the target in the inode */
        memcpy(ei->i_e2i->i_block, target, target_len);
        
        /* Set the size */
        inode->size = target_len;
        ei->i_e2i->i_size = target_len;
        
        /* Write the inode */
        ret = ext2_write_inode(sb, inode);
        
        if (ret < 0) {
            ext2_unlink(dir, name);
            ext2_destroy_inode(sb, inode);
            return ret;
        }
    } else {
        /* Allocate a block for the target */
        u32 phys_block = ext2_alloc_block(inode, 0);
        
        if (phys_block == 0) {
            ext2_unlink(dir, name);
            ext2_destroy_inode(sb, inode);
            return -ENOSPC;
        }
        
        /* Set the block */
        ei->i_data[0] = phys_block;
        ei->i_e2i->i_block[0] = phys_block;
        
        /* Allocate memory for the block */
        void *block_buffer = kmalloc(sbi->s_block_size, 0);
        
        if (block_buffer == NULL) {
            printk(KERN_ERR "EXT2: Failed to allocate memory for block buffer\n");
            ext2_free_block(inode, phys_block);
            ext2_unlink(dir, name);
            ext2_destroy_inode(sb, inode);
            return -ENOMEM;
        }
        
        /* Clear the block */
        memset(block_buffer, 0, sbi->s_block_size);
        
        /* Copy the target */
        memcpy(block_buffer, target, target_len);
        
        /* Write the block */
        ret = ext2_write_block(sbi, phys_block, block_buffer);
        
        /* Free the block buffer */
        kfree(block_buffer);
        
        if (ret < 0) {
            ext2_free_block(inode, phys_block);
            ext2_unlink(dir, name);
            ext2_destroy_inode(sb, inode);
            return ret;
        }
        
        /* Set the size */
        inode->size = target_len;
        ei->i_e2i->i_size = target_len;
        
        /* Set the blocks */
        inode->blocks = sbi->s_block_size / 512;
        ei->i_e2i->i_blocks = inode->blocks;
        
        /* Write the inode */
        ret = ext2_write_inode(sb, inode);
        
        if (ret < 0) {
            ext2_free_block(inode, phys_block);
            ext2_unlink(dir, name);
            ext2_destroy_inode(sb, inode);
            return ret;
        }
    }
    
    /* Free the inode */
    ext2_destroy_inode(sb, inode);
    
    return 0;
}

/**
 * Read a symbolic link
 * 
 * @param inode Symbolic link inode
 * @param buffer Buffer to read into
 * @param size Size of the buffer
 * @return 0 on success, negative error code on failure
 */
error_t ext2_readlink(struct inode *inode, char *buffer, size_t size) {
    /* Check if the inode exists */
    if (inode == NULL) {
        printk(KERN_ERR "EXT2: Inode does not exist\n");
        return -ENOENT;
    }
    
    /* Check if the inode is a symbolic link */
    if (inode->type != FILE_TYPE_SYMLINK) {
        printk(KERN_ERR "EXT2: Not a symbolic link\n");
        return -EINVAL;
    }
    
    /* Check if the buffer is valid */
    if (buffer == NULL) {
        printk(KERN_ERR "EXT2: Invalid buffer\n");
        return -EINVAL;
    }
    
    /* Get the Ext2 inode info */
    ext2_inode_info_t *ei = (ext2_inode_info_t *)inode->fs_data;
    
    /* Get the superblock */
    super_block_t *sb = inode->i_ops->get_super(inode);
    
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Check if the target is stored in the inode */
    if (inode->size <= 60) {
        /* Copy the target */
        size_t len = inode->size < size ? inode->size : size - 1;
        memcpy(buffer, ei->i_e2i->i_block, len);
        buffer[len] = '\0';
        
        return 0;
    } else {
        /* Get the block */
        u32 phys_block = ei->i_data[0];
        
        if (phys_block == 0) {
            printk(KERN_ERR "EXT2: Invalid block\n");
            return -EIO;
        }
        
        /* Allocate memory for the block */
        void *block_buffer = kmalloc(sbi->s_block_size, 0);
        
        if (block_buffer == NULL) {
            printk(KERN_ERR "EXT2: Failed to allocate memory for block buffer\n");
            return -ENOMEM;
        }
        
        /* Read the block */
        int ret = ext2_read_block(sbi, phys_block, block_buffer);
        
        if (ret < 0) {
            kfree(block_buffer);
            return ret;
        }
        
        /* Copy the target */
        size_t len = inode->size < size ? inode->size : size - 1;
        memcpy(buffer, block_buffer, len);
        buffer[len] = '\0';
        
        /* Free the block buffer */
        kfree(block_buffer);
        
        return 0;
    }
}

/**
 * Follow a symbolic link
 * 
 * @param inode Symbolic link inode
 * @param target Pointer to store the target inode
 * @return 0 on success, negative error code on failure
 */
error_t ext2_follow_link(struct inode *inode, struct inode **target) {
    /* Check if the inode exists */
    if (inode == NULL) {
        printk(KERN_ERR "EXT2: Inode does not exist\n");
        return -ENOENT;
    }
    
    /* Check if the inode is a symbolic link */
    if (inode->type != FILE_TYPE_SYMLINK) {
        printk(KERN_ERR "EXT2: Not a symbolic link\n");
        return -EINVAL;
    }
    
    /* Check if the target is valid */
    if (target == NULL) {
        printk(KERN_ERR "EXT2: Invalid target\n");
        return -EINVAL;
    }
    
    /* Allocate memory for the target path */
    char *path = kmalloc(inode->size + 1, 0);
    
    if (path == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for path\n");
        return -ENOMEM;
    }
    
    /* Read the symbolic link */
    error_t ret = ext2_readlink(inode, path, inode->size + 1);
    
    if (ret < 0) {
        kfree(path);
        return ret;
    }
    
    /* Look up the target */
    *target = fs_lookup(path);
    
    /* Free the path */
    kfree(path);
    
    if (*target == NULL) {
        return -ENOENT;
    }
    
    return 0;
}
