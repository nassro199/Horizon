/**
 * readdir.c - Ext2 readdir operations
 *
 * This file contains the implementation of Ext2 readdir operations.
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
 * Read directory entries
 * 
 * @param file Directory to read from
 * @param dirent_buf Buffer to read into
 * @param count Number of bytes to read
 * @return Number of bytes read, or negative error code on failure
 */
ssize_t ext2_readdir_file(file_t *file, void *dirent_buf, size_t count) {
    /* Check if the file exists */
    if (file == NULL) {
        printk(KERN_ERR "EXT2: File does not exist\n");
        return -ENOENT;
    }
    
    /* Check if the file is a directory */
    if (file->inode == NULL || file->inode->type != FILE_TYPE_DIRECTORY) {
        printk(KERN_ERR "EXT2: Not a directory\n");
        return -ENOTDIR;
    }
    
    /* Check if the buffer is valid */
    if (dirent_buf == NULL) {
        printk(KERN_ERR "EXT2: Invalid buffer\n");
        return -EINVAL;
    }
    
    /* Read a directory entry */
    dirent_t dirent;
    error_t ret = ext2_readdir_dir(file, &dirent);
    
    if (ret < 0) {
        return ret;
    }
    
    /* Check if there is enough space in the buffer */
    size_t dirent_size = sizeof(dirent_t);
    
    if (count < dirent_size) {
        return -EINVAL;
    }
    
    /* Copy the directory entry to the buffer */
    memcpy(dirent_buf, &dirent, dirent_size);
    
    return dirent_size;
}
