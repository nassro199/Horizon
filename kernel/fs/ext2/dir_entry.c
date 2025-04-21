/**
 * dir_entry.c - Ext2 directory entry operations
 *
 * This file contains the implementation of Ext2 directory entry operations.
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
 * Add an entry to a directory
 * 
 * @param dir Directory inode
 * @param name Name of the entry
 * @param ino Inode number of the entry
 * @param type Type of the entry
 * @return 0 on success, negative error code on failure
 */
error_t ext2_add_entry(struct inode *dir, const char *name, u32 ino, file_type_t type) {
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
    
    /* Get the name length */
    u32 name_len = strlen(name);
    
    /* Check if the name is too long */
    if (name_len > 255) {
        printk(KERN_ERR "EXT2: Name too long\n");
        return -ENAMETOOLONG;
    }
    
    /* Get the superblock */
    super_block_t *sb = dir->i_ops->get_super(dir);
    
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Calculate the block size */
    u32 block_size = sbi->s_block_size;
    
    /* Calculate the entry size */
    u32 entry_size = sizeof(ext2_dir_entry_t) - 255 + name_len;
    
    /* Align the entry size to a 4-byte boundary */
    entry_size = (entry_size + 3) & ~3;
    
    /* Calculate the number of blocks */
    u32 num_blocks = (dir->size + block_size - 1) / block_size;
    
    /* Allocate memory for a block */
    void *block_buffer = kmalloc(block_size, 0);
    
    if (block_buffer == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for block buffer\n");
        return -ENOMEM;
    }
    
    /* Search for a free entry */
    for (u32 block_num = 0; block_num < num_blocks; block_num++) {
        /* Get the physical block number */
        u32 phys_block = ext2_get_block(dir, block_num);
        
        if (phys_block == 0) {
            /* Sparse file, allocate a new block */
            phys_block = ext2_alloc_block(dir, block_num);
            
            if (phys_block == 0) {
                kfree(block_buffer);
                return -ENOSPC;
            }
            
            /* Clear the block */
            memset(block_buffer, 0, block_size);
            
            /* Create a single entry that spans the entire block */
            ext2_dir_entry_t *entry = (ext2_dir_entry_t *)block_buffer;
            entry->inode = 0;
            entry->rec_len = block_size;
            entry->name_len = 0;
            entry->file_type = 0;
            
            /* Write the block */
            int ret = ext2_write_block(sbi, phys_block, block_buffer);
            
            if (ret < 0) {
                kfree(block_buffer);
                return ret;
            }
        } else {
            /* Read the block */
            int ret = ext2_read_block(sbi, phys_block, block_buffer);
            
            if (ret < 0) {
                kfree(block_buffer);
                return ret;
            }
        }
        
        /* Search for a free entry in this block */
        u32 offset = 0;
        
        while (offset < block_size) {
            /* Get the directory entry */
            ext2_dir_entry_t *entry = (ext2_dir_entry_t *)((u8 *)block_buffer + offset);
            
            /* Check if the entry is free or has enough space */
            if (entry->inode == 0 || entry->rec_len >= entry_size + ((sizeof(ext2_dir_entry_t) - 255 + entry->name_len + 3) & ~3)) {
                /* Found a free entry */
                u32 actual_size = (sizeof(ext2_dir_entry_t) - 255 + entry->name_len + 3) & ~3;
                u32 free_space = entry->rec_len - actual_size;
                
                if (entry->inode != 0) {
                    /* Split the entry */
                    entry->rec_len = actual_size;
                    
                    /* Create a new entry */
                    ext2_dir_entry_t *new_entry = (ext2_dir_entry_t *)((u8 *)entry + actual_size);
                    new_entry->inode = ino;
                    new_entry->rec_len = free_space;
                    new_entry->name_len = name_len;
                    new_entry->file_type = type == FILE_TYPE_REGULAR ? EXT2_FT_REG_FILE :
                                          type == FILE_TYPE_DIRECTORY ? EXT2_FT_DIR :
                                          type == FILE_TYPE_SYMLINK ? EXT2_FT_SYMLINK :
                                          type == FILE_TYPE_BLOCK_DEVICE ? EXT2_FT_BLKDEV :
                                          type == FILE_TYPE_CHAR_DEVICE ? EXT2_FT_CHRDEV :
                                          type == FILE_TYPE_PIPE ? EXT2_FT_FIFO :
                                          type == FILE_TYPE_SOCKET ? EXT2_FT_SOCK :
                                          EXT2_FT_UNKNOWN;
                    memcpy(new_entry->name, name, name_len);
                } else {
                    /* Use the existing entry */
                    entry->inode = ino;
                    entry->name_len = name_len;
                    entry->file_type = type == FILE_TYPE_REGULAR ? EXT2_FT_REG_FILE :
                                      type == FILE_TYPE_DIRECTORY ? EXT2_FT_DIR :
                                      type == FILE_TYPE_SYMLINK ? EXT2_FT_SYMLINK :
                                      type == FILE_TYPE_BLOCK_DEVICE ? EXT2_FT_BLKDEV :
                                      type == FILE_TYPE_CHAR_DEVICE ? EXT2_FT_CHRDEV :
                                      type == FILE_TYPE_PIPE ? EXT2_FT_FIFO :
                                      type == FILE_TYPE_SOCKET ? EXT2_FT_SOCK :
                                      EXT2_FT_UNKNOWN;
                    memcpy(entry->name, name, name_len);
                }
                
                /* Write the block */
                int ret = ext2_write_block(sbi, phys_block, block_buffer);
                
                if (ret < 0) {
                    kfree(block_buffer);
                    return ret;
                }
                
                /* Update the directory size if necessary */
                if (block_num * block_size + block_size > dir->size) {
                    dir->size = block_num * block_size + block_size;
                    ext2_write_inode(sb, dir);
                }
                
                /* Free the block buffer */
                kfree(block_buffer);
                
                return 0;
            }
            
            /* Move to the next entry */
            offset += entry->rec_len;
        }
    }
    
    /* No free entry found, allocate a new block */
    u32 phys_block = ext2_alloc_block(dir, num_blocks);
    
    if (phys_block == 0) {
        kfree(block_buffer);
        return -ENOSPC;
    }
    
    /* Clear the block */
    memset(block_buffer, 0, block_size);
    
    /* Create a new entry */
    ext2_dir_entry_t *entry = (ext2_dir_entry_t *)block_buffer;
    entry->inode = ino;
    entry->rec_len = block_size;
    entry->name_len = name_len;
    entry->file_type = type == FILE_TYPE_REGULAR ? EXT2_FT_REG_FILE :
                      type == FILE_TYPE_DIRECTORY ? EXT2_FT_DIR :
                      type == FILE_TYPE_SYMLINK ? EXT2_FT_SYMLINK :
                      type == FILE_TYPE_BLOCK_DEVICE ? EXT2_FT_BLKDEV :
                      type == FILE_TYPE_CHAR_DEVICE ? EXT2_FT_CHRDEV :
                      type == FILE_TYPE_PIPE ? EXT2_FT_FIFO :
                      type == FILE_TYPE_SOCKET ? EXT2_FT_SOCK :
                      EXT2_FT_UNKNOWN;
    memcpy(entry->name, name, name_len);
    
    /* Write the block */
    int ret = ext2_write_block(sbi, phys_block, block_buffer);
    
    if (ret < 0) {
        kfree(block_buffer);
        return ret;
    }
    
    /* Update the directory size */
    dir->size = (num_blocks + 1) * block_size;
    ext2_write_inode(sb, dir);
    
    /* Free the block buffer */
    kfree(block_buffer);
    
    return 0;
}

/**
 * Remove an entry from a directory
 * 
 * @param dir Directory inode
 * @param name Name of the entry to remove
 * @return 0 on success, negative error code on failure
 */
error_t ext2_remove_entry(struct inode *dir, const char *name) {
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
    
    /* Get the name length */
    u32 name_len = strlen(name);
    
    /* Check if the name is too long */
    if (name_len > 255) {
        printk(KERN_ERR "EXT2: Name too long\n");
        return -ENAMETOOLONG;
    }
    
    /* Get the superblock */
    super_block_t *sb = dir->i_ops->get_super(dir);
    
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Calculate the block size */
    u32 block_size = sbi->s_block_size;
    
    /* Calculate the number of blocks */
    u32 num_blocks = (dir->size + block_size - 1) / block_size;
    
    /* Allocate memory for a block */
    void *block_buffer = kmalloc(block_size, 0);
    
    if (block_buffer == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for block buffer\n");
        return -ENOMEM;
    }
    
    /* Search for the entry */
    for (u32 block_num = 0; block_num < num_blocks; block_num++) {
        /* Get the physical block number */
        u32 phys_block = ext2_get_block(dir, block_num);
        
        if (phys_block == 0) {
            /* Sparse file, skip */
            continue;
        }
        
        /* Read the block */
        int ret = ext2_read_block(sbi, phys_block, block_buffer);
        
        if (ret < 0) {
            kfree(block_buffer);
            return ret;
        }
        
        /* Search for the entry in this block */
        u32 offset = 0;
        ext2_dir_entry_t *prev_entry = NULL;
        
        while (offset < block_size) {
            /* Get the directory entry */
            ext2_dir_entry_t *entry = (ext2_dir_entry_t *)((u8 *)block_buffer + offset);
            
            /* Check if the entry is valid */
            if (entry->inode != 0 && entry->name_len == name_len && strncmp(entry->name, name, name_len) == 0) {
                /* Found the entry */
                if (prev_entry != NULL) {
                    /* Merge with the previous entry */
                    prev_entry->rec_len += entry->rec_len;
                } else {
                    /* Mark the entry as free */
                    entry->inode = 0;
                }
                
                /* Write the block */
                ret = ext2_write_block(sbi, phys_block, block_buffer);
                
                if (ret < 0) {
                    kfree(block_buffer);
                    return ret;
                }
                
                /* Free the block buffer */
                kfree(block_buffer);
                
                return 0;
            }
            
            /* Move to the next entry */
            prev_entry = entry;
            offset += entry->rec_len;
        }
    }
    
    /* Entry not found */
    kfree(block_buffer);
    
    return -ENOENT;
}

/**
 * Check if a directory is empty
 * 
 * @param dir Directory inode
 * @return 1 if empty, 0 if not empty, negative error code on failure
 */
int ext2_is_dir_empty(struct inode *dir) {
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
    
    /* Get the superblock */
    super_block_t *sb = dir->i_ops->get_super(dir);
    
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Calculate the block size */
    u32 block_size = sbi->s_block_size;
    
    /* Calculate the number of blocks */
    u32 num_blocks = (dir->size + block_size - 1) / block_size;
    
    /* Allocate memory for a block */
    void *block_buffer = kmalloc(block_size, 0);
    
    if (block_buffer == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for block buffer\n");
        return -ENOMEM;
    }
    
    /* Search for entries */
    for (u32 block_num = 0; block_num < num_blocks; block_num++) {
        /* Get the physical block number */
        u32 phys_block = ext2_get_block(dir, block_num);
        
        if (phys_block == 0) {
            /* Sparse file, skip */
            continue;
        }
        
        /* Read the block */
        int ret = ext2_read_block(sbi, phys_block, block_buffer);
        
        if (ret < 0) {
            kfree(block_buffer);
            return ret;
        }
        
        /* Search for entries in this block */
        u32 offset = 0;
        
        while (offset < block_size) {
            /* Get the directory entry */
            ext2_dir_entry_t *entry = (ext2_dir_entry_t *)((u8 *)block_buffer + offset);
            
            /* Check if the entry is valid */
            if (entry->inode != 0) {
                /* Check if the entry is "." or ".." */
                if (entry->name_len == 1 && entry->name[0] == '.') {
                    /* "." entry, skip */
                } else if (entry->name_len == 2 && entry->name[0] == '.' && entry->name[1] == '.') {
                    /* ".." entry, skip */
                } else {
                    /* Found a regular entry */
                    kfree(block_buffer);
                    return 0;
                }
            }
            
            /* Move to the next entry */
            offset += entry->rec_len;
        }
    }
    
    /* No entries found */
    kfree(block_buffer);
    
    return 1;
}
