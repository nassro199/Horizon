/**
 * ext2_ops.c - Ext2 operations
 *
 * This file contains the definitions of Ext2 operations.
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

/* Ext2 inode operations */
struct inode_operations ext2_inode_ops = {
    .lookup = ext2_lookup,
    .create = ext2_create,
    .link = ext2_link,
    .unlink = ext2_unlink,
    .symlink = ext2_symlink,
    .mkdir = ext2_mkdir,
    .rmdir = ext2_rmdir,
    .rename = ext2_rename,
    .readlink = ext2_readlink,
    .follow_link = ext2_follow_link,
    .truncate = ext2_truncate,
    .permission = ext2_permission,
    .setattr = ext2_setattr,
    .getattr = ext2_getattr,
    .get_super = ext2_get_super_from_inode
};

/* Ext2 file operations */
struct file_operations ext2_file_ops = {
    .read = ext2_read,
    .write = ext2_write,
    .open = ext2_open_file,
    .close = ext2_close,
    .seek = ext2_seek,
    .flush = ext2_flush,
    .fsync = ext2_fsync,
    .ioctl = ext2_ioctl,
    .mmap = ext2_mmap,
    .readdir = ext2_readdir_file
};

/* Ext2 directory operations */
struct file_operations ext2_dir_ops = {
    .read = ext2_read_dir,
    .open = ext2_open_dir,
    .close = ext2_close_dir,
    .readdir = ext2_readdir_dir
};

/* Ext2 superblock operations */
struct super_operations ext2_super_ops = {
    .alloc_inode = ext2_alloc_inode,
    .destroy_inode = ext2_destroy_inode,
    .write_inode = ext2_write_inode,
    .read_inode = ext2_read_inode,
    .put_super = ext2_put_super,
    .write_super = ext2_write_super,
    .statfs = ext2_statfs,
    .remount_fs = ext2_remount_fs
};

/**
 * Get the superblock from an inode
 * 
 * @param inode Inode to get superblock for
 * @return Pointer to the superblock, or NULL on failure
 */
struct super_block *ext2_get_super_from_inode(struct inode *inode) {
    /* Check if the inode exists */
    if (inode == NULL) {
        printk(KERN_ERR "EXT2: Inode does not exist\n");
        return NULL;
    }
    
    /* Get the mount point */
    for (int i = 0; i < mount_count; i++) {
        /* Check if the mount point is valid */
        if (mounts[i].root != NULL) {
            /* Check if the inode is on this file system */
            if (inode->inode_num >= mounts[i].root->inode_num && 
                inode->inode_num < mounts[i].root->inode_num + 1000000) {
                /* Found the mount point */
                return mounts[i].super;
            }
        }
    }
    
    /* Not found */
    return NULL;
}
