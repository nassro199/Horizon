/**
 * create.c - Ext2 file creation operations
 *
 * This file contains the implementation of Ext2 file creation operations.
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

/* External declarations */
extern int ext2_read_block(ext2_sb_info_t *sb, u32 block, void *buffer);
extern int ext2_write_block(ext2_sb_info_t *sb, u32 block, void *buffer);

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/**
 * Create a new inode
 *
 * @param dir Parent directory inode
 * @param name Name of the new file
 * @param mode File mode
 * @param inode Pointer to store the new inode
 * @return 0 on success, negative error code on failure
 */
error_t ext2_create(struct inode *dir, const char *name, u32 mode, struct inode **inode) {
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

    /* Get the superblock */
    super_block_t *sb = dir->i_ops->get_super(dir);

    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;

    /* Allocate a new inode */
    u32 ino = ext2_new_inode(dir);

    if (ino == 0) {
        printk(KERN_ERR "EXT2: Failed to allocate inode\n");
        return -ENOSPC;
    }

    /* Create a new inode */
    struct inode *new_inode = ext2_alloc_inode(sb);

    if (new_inode == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for inode\n");
        ext2_free_inode(dir, ino);
        return -ENOMEM;
    }

    /* Initialize the inode */
    new_inode->inode_num = ino;
    new_inode->type = (mode & EXT2_S_IFMT) == EXT2_S_IFREG ? FILE_TYPE_REGULAR :
                      (mode & EXT2_S_IFMT) == EXT2_S_IFDIR ? FILE_TYPE_DIRECTORY :
                      (mode & EXT2_S_IFMT) == EXT2_S_IFLNK ? FILE_TYPE_SYMLINK :
                      (mode & EXT2_S_IFMT) == EXT2_S_IFBLK ? FILE_TYPE_BLOCK_DEVICE :
                      (mode & EXT2_S_IFMT) == EXT2_S_IFCHR ? FILE_TYPE_CHAR_DEVICE :
                      (mode & EXT2_S_IFMT) == EXT2_S_IFIFO ? FILE_TYPE_PIPE :
                      (mode & EXT2_S_IFMT) == EXT2_S_IFSOCK ? FILE_TYPE_SOCKET :
                      FILE_TYPE_REGULAR;
    new_inode->permissions = mode & 0xFFF;
    new_inode->uid = 0; /* Root user */
    new_inode->gid = 0; /* Root group */
    new_inode->size = 0;
    new_inode->blocks = 0;
    new_inode->atime = time_get_unix_time();
    new_inode->mtime = new_inode->atime;
    new_inode->ctime = new_inode->atime;
    new_inode->links = 1;

    /* Get the Ext2 inode info */
    ext2_inode_info_t *ei = (ext2_inode_info_t *)new_inode->fs_data;

    /* Allocate memory for the Ext2 inode */
    ei->i_e2i = kmalloc(sizeof(ext2_inode_t), 0);

    if (ei->i_e2i == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for Ext2 inode\n");
        ext2_destroy_inode(sb, new_inode);
        ext2_free_inode(dir, ino);
        return -ENOMEM;
    }

    /* Initialize the Ext2 inode */
    memset(ei->i_e2i, 0, sizeof(ext2_inode_t));

    /* Write the inode */
    error_t ret = ext2_write_inode(sb, new_inode);

    if (ret < 0) {
        ext2_destroy_inode(sb, new_inode);
        ext2_free_inode(dir, ino);
        return ret;
    }

    /* Add the entry to the directory */
    ret = ext2_add_entry(dir, name, ino, new_inode->type);

    if (ret < 0) {
        ext2_destroy_inode(sb, new_inode);
        ext2_free_inode(dir, ino);
        return ret;
    }

    /* Return the inode */
    *inode = new_inode;

    return 0;
}

/**
 * Create a directory
 *
 * @param dir Parent directory inode
 * @param name Name of the new directory
 * @param mode Directory mode
 * @return 0 on success, negative error code on failure
 */
error_t ext2_mkdir(struct inode *dir, const char *name, u32 mode) {
    /* Set the directory mode */
    mode = (mode & 0xFFF) | EXT2_S_IFDIR;

    /* Create the directory inode */
    struct inode *inode;
    error_t ret = ext2_create(dir, name, mode, &inode);

    if (ret < 0) {
        return ret;
    }

    /* Create the "." entry */
    ret = ext2_add_entry(inode, ".", inode->inode_num, FILE_TYPE_DIRECTORY);

    if (ret < 0) {
        ext2_unlink(dir, name);
        ext2_destroy_inode(NULL, inode);
        return ret;
    }

    /* Create the ".." entry */
    ret = ext2_add_entry(inode, "..", dir->inode_num, FILE_TYPE_DIRECTORY);

    if (ret < 0) {
        ext2_unlink(dir, name);
        ext2_destroy_inode(NULL, inode);
        return ret;
    }

    /* Increment the link count of the parent directory */
    dir->links++;
    ext2_write_inode(dir->i_ops->get_super(dir), dir);

    /* Free the inode */
    ext2_destroy_inode(NULL, inode);

    return 0;
}

/**
 * Remove a directory
 *
 * @param dir Parent directory inode
 * @param name Name of the directory to remove
 * @return 0 on success, negative error code on failure
 */
error_t ext2_rmdir(struct inode *dir, const char *name) {
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

    /* Check if the directory exists */
    struct inode *inode = ext2_lookup(dir, name);

    if (inode == NULL) {
        return -ENOENT;
    }

    /* Check if the directory is a directory */
    if (inode->type != FILE_TYPE_DIRECTORY) {
        ext2_destroy_inode(NULL, inode);
        return -ENOTDIR;
    }

    /* Check if the directory is empty */
    if (ext2_is_dir_empty(inode) != 1) {
        ext2_destroy_inode(NULL, inode);
        return -ENOTEMPTY;
    }

    /* Remove the directory entry */
    error_t ret = ext2_remove_entry(dir, name);

    if (ret < 0) {
        ext2_destroy_inode(NULL, inode);
        return ret;
    }

    /* Decrement the link count of the parent directory */
    dir->links--;
    ext2_write_inode(dir->i_ops->get_super(dir), dir);

    /* Free the inode */
    ext2_free_inode(dir, inode->inode_num);
    ext2_destroy_inode(NULL, inode);

    return 0;
}
