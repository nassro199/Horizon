/**
 * rename.c - Ext2 rename operations
 *
 * This file contains the implementation of Ext2 rename operations.
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
 * Rename a file or directory
 * 
 * @param old_dir Old parent directory inode
 * @param old_name Old name
 * @param new_dir New parent directory inode
 * @param new_name New name
 * @return 0 on success, negative error code on failure
 */
error_t ext2_rename(struct inode *old_dir, const char *old_name, struct inode *new_dir, const char *new_name) {
    /* Check if the old directory exists */
    if (old_dir == NULL) {
        printk(KERN_ERR "EXT2: Old directory does not exist\n");
        return -ENOENT;
    }
    
    /* Check if the old directory is a directory */
    if (old_dir->type != FILE_TYPE_DIRECTORY) {
        printk(KERN_ERR "EXT2: Old directory is not a directory\n");
        return -ENOTDIR;
    }
    
    /* Check if the new directory exists */
    if (new_dir == NULL) {
        printk(KERN_ERR "EXT2: New directory does not exist\n");
        return -ENOENT;
    }
    
    /* Check if the new directory is a directory */
    if (new_dir->type != FILE_TYPE_DIRECTORY) {
        printk(KERN_ERR "EXT2: New directory is not a directory\n");
        return -ENOTDIR;
    }
    
    /* Check if the old name is valid */
    if (old_name == NULL || old_name[0] == '\0') {
        printk(KERN_ERR "EXT2: Invalid old name\n");
        return -EINVAL;
    }
    
    /* Check if the new name is valid */
    if (new_name == NULL || new_name[0] == '\0') {
        printk(KERN_ERR "EXT2: Invalid new name\n");
        return -EINVAL;
    }
    
    /* Get the superblock */
    super_block_t *sb = old_dir->i_ops->get_super(old_dir);
    
    /* Check if the file system is read-only */
    if (sb->flags & MOUNT_READ_ONLY) {
        printk(KERN_ERR "EXT2: File system is read-only\n");
        return -EROFS;
    }
    
    /* Look up the old inode */
    struct inode *inode = ext2_lookup(old_dir, old_name);
    
    if (inode == NULL) {
        return -ENOENT;
    }
    
    /* Check if the new file already exists */
    struct inode *new_inode = ext2_lookup(new_dir, new_name);
    
    if (new_inode != NULL) {
        /* Check if the new file is a directory */
        if (new_inode->type == FILE_TYPE_DIRECTORY) {
            /* Check if the directory is empty */
            if (ext2_is_dir_empty(new_inode) != 1) {
                ext2_destroy_inode(NULL, inode);
                ext2_destroy_inode(NULL, new_inode);
                return -ENOTEMPTY;
            }
            
            /* Decrement the link count of the new directory's parent */
            new_dir->links--;
            ext2_write_inode(sb, new_dir);
        }
        
        /* Remove the new file */
        error_t ret = ext2_remove_entry(new_dir, new_name);
        
        if (ret < 0) {
            ext2_destroy_inode(NULL, inode);
            ext2_destroy_inode(NULL, new_inode);
            return ret;
        }
        
        /* Free the inode */
        ret = ext2_free_inode(new_dir, new_inode->inode_num);
        
        if (ret < 0) {
            ext2_destroy_inode(NULL, inode);
            ext2_destroy_inode(NULL, new_inode);
            return ret;
        }
        
        /* Free the new inode */
        ext2_destroy_inode(NULL, new_inode);
    }
    
    /* Add the entry to the new directory */
    error_t ret = ext2_add_entry(new_dir, new_name, inode->inode_num, inode->type);
    
    if (ret < 0) {
        ext2_destroy_inode(NULL, inode);
        return ret;
    }
    
    /* Remove the entry from the old directory */
    ret = ext2_remove_entry(old_dir, old_name);
    
    if (ret < 0) {
        /* Try to remove the entry from the new directory */
        ext2_remove_entry(new_dir, new_name);
        ext2_destroy_inode(NULL, inode);
        return ret;
    }
    
    /* Check if the file is a directory */
    if (inode->type == FILE_TYPE_DIRECTORY) {
        /* Update the ".." entry */
        file_t *dir = fs_opendir(new_name);
        
        if (dir != NULL) {
            /* Find the ".." entry */
            dirent_t dirent;
            
            while (fs_readdir(dir, &dirent) == 0) {
                if (strcmp(dirent.name, "..") == 0) {
                    /* Found the ".." entry */
                    break;
                }
            }
            
            /* Close the directory */
            fs_closedir(dir);
            
            /* Update the ".." entry */
            ret = ext2_remove_entry(inode, "..");
            
            if (ret < 0) {
                ext2_destroy_inode(NULL, inode);
                return ret;
            }
            
            ret = ext2_add_entry(inode, "..", new_dir->inode_num, FILE_TYPE_DIRECTORY);
            
            if (ret < 0) {
                ext2_destroy_inode(NULL, inode);
                return ret;
            }
            
            /* Update the link counts */
            old_dir->links--;
            new_dir->links++;
            
            /* Write the inodes */
            ext2_write_inode(sb, old_dir);
            ext2_write_inode(sb, new_dir);
        }
    }
    
    /* Update the change time */
    inode->ctime = time_get_unix_time();
    ext2_write_inode(sb, inode);
    
    /* Free the inode */
    ext2_destroy_inode(NULL, inode);
    
    return 0;
}
