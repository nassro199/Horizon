/**
 * dir.c - Ext2 directory operations
 *
 * This file contains the implementation of Ext2 directory operations.
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
 * Open a directory
 * 
 * @param file Directory to open
 * @param flags Open flags
 * @return 0 on success, negative error code on failure
 */
error_t ext2_open_dir(file_t *file, u32 flags) {
    /* Check if the directory exists */
    if (file->inode == NULL) {
        printk(KERN_ERR "EXT2: Directory does not exist\n");
        return -ENOENT;
    }
    
    /* Check if the file is a directory */
    if (file->inode->type != FILE_TYPE_DIRECTORY) {
        printk(KERN_ERR "EXT2: Not a directory\n");
        return -ENOTDIR;
    }
    
    /* Set the file operations */
    file->f_ops = &ext2_dir_ops;
    
    /* Set the file flags */
    file->flags = flags;
    
    /* Set the file position */
    file->position = 0;
    
    return 0;
}

/**
 * Close a directory
 * 
 * @param file Directory to close
 * @return 0 on success, negative error code on failure
 */
error_t ext2_close_dir(file_t *file) {
    /* Nothing to do */
    return 0;
}

/**
 * Read from a directory
 * 
 * @param file Directory to read from
 * @param buffer Buffer to read into
 * @param size Number of bytes to read
 * @return Number of bytes read, or negative error code on failure
 */
ssize_t ext2_read_dir(file_t *file, void *buffer, size_t size) {
    /* Check if the file is open for reading */
    if (!(file->flags & FILE_OPEN_READ)) {
        printk(KERN_ERR "EXT2: Directory not open for reading\n");
        return -EBADF;
    }
    
    /* Check if we're at the end of the directory */
    if (file->position >= file->inode->size) {
        return 0;
    }
    
    /* Get the Ext2 inode info */
    ext2_inode_info_t *ei = (ext2_inode_info_t *)file->inode->fs_data;
    
    /* Get the superblock */
    super_block_t *sb = file->dentry->inode->i_ops->get_super(file->dentry->inode);
    
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Calculate the number of bytes to read */
    size_t bytes_to_read = size;
    
    if (file->position + bytes_to_read > file->inode->size) {
        bytes_to_read = file->inode->size - file->position;
    }
    
    /* Calculate the block size */
    u32 block_size = sbi->s_block_size;
    
    /* Calculate the starting block */
    u32 start_block = file->position / block_size;
    
    /* Calculate the offset within the starting block */
    u32 start_offset = file->position % block_size;
    
    /* Calculate the ending block */
    u32 end_block = (file->position + bytes_to_read - 1) / block_size;
    
    /* Allocate memory for a block */
    void *block_buffer = kmalloc(block_size, 0);
    
    if (block_buffer == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for block buffer\n");
        return -ENOMEM;
    }
    
    /* Read the blocks */
    size_t bytes_read = 0;
    
    for (u32 block_num = start_block; block_num <= end_block; block_num++) {
        /* Get the physical block number */
        u32 phys_block = ext2_get_block(file->inode, block_num);
        
        if (phys_block == 0) {
            /* Sparse file, fill with zeros */
            memset(block_buffer, 0, block_size);
        } else {
            /* Read the block */
            int ret = ext2_read_block(sbi, phys_block, block_buffer);
            
            if (ret < 0) {
                kfree(block_buffer);
                return ret;
            }
        }
        
        /* Calculate the offset within this block */
        u32 offset = (block_num == start_block) ? start_offset : 0;
        
        /* Calculate the number of bytes to copy from this block */
        u32 bytes_to_copy = block_size - offset;
        
        if (block_num == end_block) {
            bytes_to_copy = file->position + bytes_to_read - block_num * block_size;
        }
        
        /* Copy the data */
        memcpy((u8 *)buffer + bytes_read, (u8 *)block_buffer + offset, bytes_to_copy);
        
        /* Update the number of bytes read */
        bytes_read += bytes_to_copy;
    }
    
    /* Free the block buffer */
    kfree(block_buffer);
    
    /* Update the file position */
    file->position += bytes_read;
    
    return bytes_read;
}

/**
 * Read a directory entry
 * 
 * @param file Directory to read from
 * @param dirent Directory entry to read into
 * @return 0 on success, negative error code on failure
 */
error_t ext2_readdir_dir(file_t *file, struct dirent *dirent) {
    /* Check if the file is open for reading */
    if (!(file->flags & FILE_OPEN_READ)) {
        printk(KERN_ERR "EXT2: Directory not open for reading\n");
        return -EBADF;
    }
    
    /* Check if we're at the end of the directory */
    if (file->position >= file->inode->size) {
        return -ENOENT;
    }
    
    /* Get the Ext2 inode info */
    ext2_inode_info_t *ei = (ext2_inode_info_t *)file->inode->fs_data;
    
    /* Get the superblock */
    super_block_t *sb = file->dentry->inode->i_ops->get_super(file->dentry->inode);
    
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Calculate the block size */
    u32 block_size = sbi->s_block_size;
    
    /* Calculate the starting block */
    u32 start_block = file->position / block_size;
    
    /* Calculate the offset within the starting block */
    u32 start_offset = file->position % block_size;
    
    /* Allocate memory for a block */
    void *block_buffer = kmalloc(block_size, 0);
    
    if (block_buffer == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for block buffer\n");
        return -ENOMEM;
    }
    
    /* Get the physical block number */
    u32 phys_block = ext2_get_block(file->inode, start_block);
    
    if (phys_block == 0) {
        /* Sparse file, fill with zeros */
        memset(block_buffer, 0, block_size);
    } else {
        /* Read the block */
        int ret = ext2_read_block(sbi, phys_block, block_buffer);
        
        if (ret < 0) {
            kfree(block_buffer);
            return ret;
        }
    }
    
    /* Get the directory entry */
    ext2_dir_entry_t *entry = (ext2_dir_entry_t *)((u8 *)block_buffer + start_offset);
    
    /* Check if the entry is valid */
    if (entry->inode == 0 || entry->rec_len == 0) {
        /* Skip to the next entry */
        file->position += entry->rec_len;
        
        /* Free the block buffer */
        kfree(block_buffer);
        
        /* Try again */
        return ext2_readdir_dir(file, dirent);
    }
    
    /* Fill in the directory entry */
    dirent->inode = entry->inode;
    dirent->type = entry->file_type;
    memcpy(dirent->name, entry->name, entry->name_len);
    dirent->name[entry->name_len] = '\0';
    
    /* Update the file position */
    file->position += entry->rec_len;
    
    /* Free the block buffer */
    kfree(block_buffer);
    
    return 0;
}

/**
 * Lookup a directory entry
 * 
 * @param dir Directory to look in
 * @param name Name to look for
 * @return Pointer to the inode, or NULL if not found
 */
struct inode *ext2_lookup(struct inode *dir, const char *name) {
    /* Check if the directory exists */
    if (dir == NULL) {
        printk(KERN_ERR "EXT2: Directory does not exist\n");
        return NULL;
    }
    
    /* Check if the directory is a directory */
    if (dir->type != FILE_TYPE_DIRECTORY) {
        printk(KERN_ERR "EXT2: Not a directory\n");
        return NULL;
    }
    
    /* Get the Ext2 inode info */
    ext2_inode_info_t *ei = (ext2_inode_info_t *)dir->fs_data;
    
    /* Get the superblock */
    super_block_t *sb = dir->i_ops->get_super(dir);
    
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Calculate the block size */
    u32 block_size = sbi->s_block_size;
    
    /* Allocate memory for a block */
    void *block_buffer = kmalloc(block_size, 0);
    
    if (block_buffer == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for block buffer\n");
        return NULL;
    }
    
    /* Calculate the number of blocks */
    u32 num_blocks = (dir->size + block_size - 1) / block_size;
    
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
            return NULL;
        }
        
        /* Search for the entry in this block */
        u32 offset = 0;
        
        while (offset < block_size) {
            /* Get the directory entry */
            ext2_dir_entry_t *entry = (ext2_dir_entry_t *)((u8 *)block_buffer + offset);
            
            /* Check if the entry is valid */
            if (entry->inode == 0 || entry->rec_len == 0) {
                /* Skip to the next entry */
                offset += entry->rec_len;
                continue;
            }
            
            /* Check if the name matches */
            if (entry->name_len == strlen(name) && strncmp(entry->name, name, entry->name_len) == 0) {
                /* Found the entry */
                inode_t *inode = kmalloc(sizeof(inode_t), 0);
                
                if (inode == NULL) {
                    printk(KERN_ERR "EXT2: Failed to allocate memory for inode\n");
                    kfree(block_buffer);
                    return NULL;
                }
                
                /* Initialize the inode */
                memset(inode, 0, sizeof(inode_t));
                
                /* Set the inode number */
                inode->inode_num = entry->inode;
                
                /* Set the inode operations */
                inode->i_ops = &ext2_inode_ops;
                
                /* Allocate memory for the Ext2 inode info */
                ext2_inode_info_t *ei = kmalloc(sizeof(ext2_inode_info_t), 0);
                
                if (ei == NULL) {
                    printk(KERN_ERR "EXT2: Failed to allocate memory for Ext2 inode info\n");
                    kfree(inode);
                    kfree(block_buffer);
                    return NULL;
                }
                
                /* Initialize the Ext2 inode info */
                memset(ei, 0, sizeof(ext2_inode_info_t));
                
                /* Set the Ext2 inode info */
                inode->fs_data = ei;
                
                /* Read the inode */
                ret = ext2_read_inode(sb, inode);
                
                if (ret < 0) {
                    kfree(ei);
                    kfree(inode);
                    kfree(block_buffer);
                    return NULL;
                }
                
                /* Free the block buffer */
                kfree(block_buffer);
                
                return inode;
            }
            
            /* Skip to the next entry */
            offset += entry->rec_len;
        }
    }
    
    /* Free the block buffer */
    kfree(block_buffer);
    
    /* Entry not found */
    return NULL;
}
