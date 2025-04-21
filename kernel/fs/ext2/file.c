/**
 * file.c - Ext2 file operations
 *
 * This file contains the implementation of Ext2 file operations.
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
 * Open a file
 * 
 * @param file File to open
 * @param flags Open flags
 * @return 0 on success, negative error code on failure
 */
error_t ext2_open_file(file_t *file, u32 flags) {
    /* Check if the file exists */
    if (file->inode == NULL) {
        printk(KERN_ERR "EXT2: File does not exist\n");
        return -ENOENT;
    }
    
    /* Check if the file is a regular file */
    if (file->inode->type != FILE_TYPE_REGULAR) {
        printk(KERN_ERR "EXT2: Not a regular file\n");
        return -EISDIR;
    }
    
    /* Set the file operations */
    file->f_ops = &ext2_file_ops;
    
    /* Set the file flags */
    file->flags = flags;
    
    /* Set the file position */
    file->position = 0;
    
    return 0;
}

/**
 * Close a file
 * 
 * @param file File to close
 * @return 0 on success, negative error code on failure
 */
error_t ext2_close(file_t *file) {
    /* Nothing to do */
    return 0;
}

/**
 * Read from a file
 * 
 * @param file File to read from
 * @param buffer Buffer to read into
 * @param size Number of bytes to read
 * @return Number of bytes read, or negative error code on failure
 */
ssize_t ext2_read(file_t *file, void *buffer, size_t size) {
    /* Check if the file is open for reading */
    if (!(file->flags & FILE_OPEN_READ)) {
        printk(KERN_ERR "EXT2: File not open for reading\n");
        return -EBADF;
    }
    
    /* Check if we're at the end of the file */
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
 * Write to a file
 * 
 * @param file File to write to
 * @param buffer Buffer to write from
 * @param size Number of bytes to write
 * @return Number of bytes written, or negative error code on failure
 */
ssize_t ext2_write(file_t *file, const void *buffer, size_t size) {
    /* Check if the file is open for writing */
    if (!(file->flags & FILE_OPEN_WRITE)) {
        printk(KERN_ERR "EXT2: File not open for writing\n");
        return -EBADF;
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
    
    /* Calculate the ending block */
    u32 end_block = (file->position + size - 1) / block_size;
    
    /* Allocate memory for a block */
    void *block_buffer = kmalloc(block_size, 0);
    
    if (block_buffer == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for block buffer\n");
        return -ENOMEM;
    }
    
    /* Write the blocks */
    size_t bytes_written = 0;
    
    for (u32 block_num = start_block; block_num <= end_block; block_num++) {
        /* Get the physical block number */
        u32 phys_block = ext2_get_block(file->inode, block_num);
        
        if (phys_block == 0) {
            /* Allocate a new block */
            phys_block = ext2_alloc_block(file->inode, block_num);
            
            if (phys_block == 0) {
                kfree(block_buffer);
                return -ENOSPC;
            }
            
            /* Clear the block */
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
        
        /* Calculate the number of bytes to copy to this block */
        u32 bytes_to_copy = block_size - offset;
        
        if (bytes_to_copy > size - bytes_written) {
            bytes_to_copy = size - bytes_written;
        }
        
        /* Copy the data */
        memcpy((u8 *)block_buffer + offset, (u8 *)buffer + bytes_written, bytes_to_copy);
        
        /* Write the block */
        int ret = ext2_write_block(sbi, phys_block, block_buffer);
        
        if (ret < 0) {
            kfree(block_buffer);
            return ret;
        }
        
        /* Update the number of bytes written */
        bytes_written += bytes_to_copy;
    }
    
    /* Free the block buffer */
    kfree(block_buffer);
    
    /* Update the file position */
    file->position += bytes_written;
    
    /* Update the file size if necessary */
    if (file->position > file->inode->size) {
        file->inode->size = file->position;
        
        /* Write the inode */
        ext2_write_inode(sb, file->inode);
    }
    
    return bytes_written;
}

/**
 * Seek within a file
 * 
 * @param file File to seek within
 * @param offset Offset to seek to
 * @param whence Seek origin
 * @return 0 on success, negative error code on failure
 */
error_t ext2_seek(file_t *file, u64 offset, int whence) {
    /* Calculate the new position */
    u64 new_position;
    
    switch (whence) {
        case SEEK_SET:
            new_position = offset;
            break;
        
        case SEEK_CUR:
            new_position = file->position + offset;
            break;
        
        case SEEK_END:
            new_position = file->inode->size + offset;
            break;
        
        default:
            return -EINVAL;
    }
    
    /* Check if the new position is valid */
    if (new_position > file->inode->size) {
        return -EINVAL;
    }
    
    /* Set the new position */
    file->position = new_position;
    
    return 0;
}

/**
 * Flush a file
 * 
 * @param file File to flush
 * @return 0 on success, negative error code on failure
 */
error_t ext2_flush(file_t *file) {
    /* Nothing to do */
    return 0;
}

/**
 * Synchronize a file
 * 
 * @param file File to synchronize
 * @return 0 on success, negative error code on failure
 */
error_t ext2_fsync(file_t *file) {
    /* Get the superblock */
    super_block_t *sb = file->dentry->inode->i_ops->get_super(file->dentry->inode);
    
    /* Write the inode */
    return ext2_write_inode(sb, file->inode);
}
