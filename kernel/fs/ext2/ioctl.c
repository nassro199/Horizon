/**
 * ioctl.c - Ext2 ioctl operations
 *
 * This file contains the implementation of Ext2 ioctl operations.
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
 * Perform an ioctl operation on a file
 * 
 * @param file File to perform ioctl on
 * @param cmd Command to perform
 * @param arg Argument for the command
 * @return 0 on success, negative error code on failure
 */
int ext2_ioctl(file_t *file, unsigned int cmd, unsigned long arg) {
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
    
    /* Get the Ext2 inode info */
    ext2_inode_info_t *ei = (ext2_inode_info_t *)file->inode->fs_data;
    
    /* Handle the command */
    switch (cmd) {
        case 0x80047601: /* EXT2_IOC_GETFLAGS */
            /* Return the flags */
            if (arg == 0) {
                return -EFAULT;
            }
            
            *(int *)arg = ei->i_flags;
            return 0;
        
        case 0x40047602: /* EXT2_IOC_SETFLAGS */
            /* Set the flags */
            if (arg == 0) {
                return -EFAULT;
            }
            
            ei->i_flags = *(int *)arg;
            
            /* Write the inode */
            return ext2_write_inode(file->inode->i_ops->get_super(file->inode), file->inode);
        
        default:
            /* Unknown command */
            return -ENOTTY;
    }
}

/**
 * Memory map a file
 * 
 * @param file File to memory map
 * @param vma Virtual memory area to map to
 * @return 0 on success, negative error code on failure
 */
int ext2_mmap(file_t *file, struct vm_area_struct *vma) {
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
    
    /* Check if the file is a regular file */
    if (file->inode->type != FILE_TYPE_REGULAR) {
        printk(KERN_ERR "EXT2: Not a regular file\n");
        return -ENODEV;
    }
    
    /* Check if the virtual memory area is valid */
    if (vma == NULL) {
        printk(KERN_ERR "EXT2: Invalid virtual memory area\n");
        return -EINVAL;
    }
    
    /* Not implemented yet */
    return -ENOSYS;
}
