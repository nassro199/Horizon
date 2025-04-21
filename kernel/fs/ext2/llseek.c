/**
 * llseek.c - Ext2 llseek operations
 *
 * This file contains the implementation of Ext2 llseek operations.
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

/**
 * Seek to a position in a file
 * 
 * @param file File to seek in
 * @param offset Offset to seek to
 * @param whence Where to seek from
 * @return New position, or negative error code on failure
 */
loff_t ext2_llseek(file_t *file, loff_t offset, int whence) {
    /* Check if the file exists */
    if (file == NULL) {
        printk(KERN_ERR "EXT2: File does not exist\n");
        return -ENOENT;
    }
    
    /* Check if the file is a regular file */
    if (file->inode == NULL) {
        printk(KERN_ERR "EXT2: File has no inode\n");
        return -EINVAL;
    }
    
    /* Get the file size */
    loff_t size = file->inode->size;
    loff_t new_pos = 0;
    
    /* Calculate the new position */
    switch (whence) {
        case SEEK_SET:
            /* Seek from the beginning of the file */
            new_pos = offset;
            break;
        
        case SEEK_CUR:
            /* Seek from the current position */
            new_pos = file->position + offset;
            break;
        
        case SEEK_END:
            /* Seek from the end of the file */
            new_pos = size + offset;
            break;
        
        default:
            /* Invalid whence */
            return -EINVAL;
    }
    
    /* Check if the new position is valid */
    if (new_pos < 0) {
        /* Cannot seek before the beginning of the file */
        return -EINVAL;
    }
    
    /* Update the file position */
    file->position = new_pos;
    
    return new_pos;
}
