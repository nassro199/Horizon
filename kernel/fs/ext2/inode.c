/**
 * inode.c - Ext2 inode operations
 *
 * This file contains the implementation of Ext2 inode operations.
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
extern struct inode_operations ext2_inode_ops;
extern int ext2_read_block(ext2_sb_info_t *sb, u32 block, void *buffer);
extern int ext2_write_block(ext2_sb_info_t *sb, u32 block, void *buffer);

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/**
 * Allocate an inode
 *
 * @param sb Superblock
 * @return Pointer to the inode, or NULL on failure
 */
struct inode *ext2_alloc_inode(struct super_block *sb) {
    /* Allocate memory for the inode */
    inode_t *inode = kmalloc(sizeof(inode_t), 0);

    if (inode == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for inode\n");
        return NULL;
    }

    /* Allocate memory for the Ext2 inode info */
    ext2_inode_info_t *ei = kmalloc(sizeof(ext2_inode_info_t), 0);

    if (ei == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for Ext2 inode info\n");
        kfree(inode);
        return NULL;
    }

    /* Initialize the inode */
    memset(inode, 0, sizeof(inode_t));
    memset(ei, 0, sizeof(ext2_inode_info_t));

    /* Set the inode operations */
    inode->i_ops = &ext2_inode_ops;

    /* Set the Ext2 inode info */
    inode->fs_data = ei;

    return inode;
}

/**
 * Destroy an inode
 *
 * @param sb Superblock
 * @param inode Inode to destroy
 */
void ext2_destroy_inode(struct super_block *sb, struct inode *inode) {
    /* Free the Ext2 inode info */
    if (inode->fs_data != NULL) {
        ext2_inode_info_t *ei = (ext2_inode_info_t *)inode->fs_data;

        if (ei->i_e2i != NULL) {
            kfree(ei->i_e2i);
        }

        kfree(ei);
    }

    /* Free the inode */
    kfree(inode);
}

/**
 * Read an inode from the device
 *
 * @param sb Superblock
 * @param inode Inode to read
 * @return 0 on success, negative error code on failure
 */
error_t ext2_read_inode(struct super_block *sb, struct inode *inode) {
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;

    /* Get the Ext2 inode info */
    ext2_inode_info_t *ei = (ext2_inode_info_t *)inode->fs_data;

    /* Calculate the block group */
    u32 block_group = (inode->inode_num - 1) / sbi->s_inodes_per_group;

    /* Calculate the index within the block group */
    u32 index = (inode->inode_num - 1) % sbi->s_inodes_per_group;

    /* Calculate the block containing the inode */
    u32 block = sbi->s_group_desc[block_group].bg_inode_table + (index * sbi->s_inode_size) / sbi->s_block_size;

    /* Calculate the offset within the block */
    u32 offset = (index * sbi->s_inode_size) % sbi->s_block_size;

    /* Allocate memory for the block */
    void *buffer = kmalloc(sbi->s_block_size, 0);

    if (buffer == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for block\n");
        return -ENOMEM;
    }

    /* Read the block */
    int ret = ext2_read_block(sbi, block, buffer);

    if (ret < 0) {
        kfree(buffer);
        return ret;
    }

    /* Allocate memory for the Ext2 inode */
    ei->i_e2i = kmalloc(sizeof(ext2_inode_t), 0);

    if (ei->i_e2i == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for Ext2 inode\n");
        kfree(buffer);
        return -ENOMEM;
    }

    /* Copy the inode */
    memcpy(ei->i_e2i, (u8 *)buffer + offset, sizeof(ext2_inode_t));

    /* Free the buffer */
    kfree(buffer);

    /* Set the inode fields */
    inode->type = (ei->i_e2i->i_mode & EXT2_S_IFMT) == EXT2_S_IFREG ? FILE_TYPE_REGULAR :
                  (ei->i_e2i->i_mode & EXT2_S_IFMT) == EXT2_S_IFDIR ? FILE_TYPE_DIRECTORY :
                  (ei->i_e2i->i_mode & EXT2_S_IFMT) == EXT2_S_IFLNK ? FILE_TYPE_SYMLINK :
                  (ei->i_e2i->i_mode & EXT2_S_IFMT) == EXT2_S_IFBLK ? FILE_TYPE_BLOCK_DEVICE :
                  (ei->i_e2i->i_mode & EXT2_S_IFMT) == EXT2_S_IFCHR ? FILE_TYPE_CHAR_DEVICE :
                  (ei->i_e2i->i_mode & EXT2_S_IFMT) == EXT2_S_IFIFO ? FILE_TYPE_PIPE :
                  (ei->i_e2i->i_mode & EXT2_S_IFMT) == EXT2_S_IFSOCK ? FILE_TYPE_SOCKET :
                  FILE_TYPE_REGULAR;

    inode->permissions = (ei->i_e2i->i_mode & 0xFFF);
    inode->uid = ei->i_e2i->i_uid;
    inode->gid = ei->i_e2i->i_gid;
    inode->size = ei->i_e2i->i_size;
    inode->blocks = ei->i_e2i->i_blocks;
    inode->atime = ei->i_e2i->i_atime;
    inode->mtime = ei->i_e2i->i_mtime;
    inode->ctime = ei->i_e2i->i_ctime;
    inode->links = ei->i_e2i->i_links_count;

    /* Set the Ext2 inode info fields */
    ei->i_block_group = block_group;

    for (int i = 0; i < 15; i++) {
        ei->i_data[i] = ei->i_e2i->i_block[i];
    }

    ei->i_flags = ei->i_e2i->i_flags;
    ei->i_faddr = ei->i_e2i->i_faddr;
    ei->i_file_acl = ei->i_e2i->i_file_acl;
    ei->i_dir_acl = ei->i_e2i->i_dir_acl;
    ei->i_dtime = ei->i_e2i->i_dtime;

    return 0;
}

/**
 * Write an inode to the device
 *
 * @param sb Superblock
 * @param inode Inode to write
 * @return 0 on success, negative error code on failure
 */
error_t ext2_write_inode(struct super_block *sb, struct inode *inode) {
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;

    /* Get the Ext2 inode info */
    ext2_inode_info_t *ei = (ext2_inode_info_t *)inode->fs_data;

    /* Calculate the block group */
    u32 block_group = (inode->inode_num - 1) / sbi->s_inodes_per_group;

    /* Calculate the index within the block group */
    u32 index = (inode->inode_num - 1) % sbi->s_inodes_per_group;

    /* Calculate the block containing the inode */
    u32 block = sbi->s_group_desc[block_group].bg_inode_table + (index * sbi->s_inode_size) / sbi->s_block_size;

    /* Calculate the offset within the block */
    u32 offset = (index * sbi->s_inode_size) % sbi->s_block_size;

    /* Allocate memory for the block */
    void *buffer = kmalloc(sbi->s_block_size, 0);

    if (buffer == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for block\n");
        return -ENOMEM;
    }

    /* Read the block */
    int ret = ext2_read_block(sbi, block, buffer);

    if (ret < 0) {
        kfree(buffer);
        return ret;
    }

    /* Update the Ext2 inode */
    ei->i_e2i->i_mode = (inode->type == FILE_TYPE_REGULAR ? EXT2_S_IFREG :
                         inode->type == FILE_TYPE_DIRECTORY ? EXT2_S_IFDIR :
                         inode->type == FILE_TYPE_SYMLINK ? EXT2_S_IFLNK :
                         inode->type == FILE_TYPE_BLOCK_DEVICE ? EXT2_S_IFBLK :
                         inode->type == FILE_TYPE_CHAR_DEVICE ? EXT2_S_IFCHR :
                         inode->type == FILE_TYPE_PIPE ? EXT2_S_IFIFO :
                         inode->type == FILE_TYPE_SOCKET ? EXT2_S_IFSOCK :
                         EXT2_S_IFREG) | (inode->permissions & 0xFFF);

    ei->i_e2i->i_uid = inode->uid;
    ei->i_e2i->i_gid = inode->gid;
    ei->i_e2i->i_size = inode->size;
    ei->i_e2i->i_blocks = inode->blocks;
    ei->i_e2i->i_atime = inode->atime;
    ei->i_e2i->i_mtime = inode->mtime;
    ei->i_e2i->i_ctime = inode->ctime;
    ei->i_e2i->i_links_count = inode->links;

    for (int i = 0; i < 15; i++) {
        ei->i_e2i->i_block[i] = ei->i_data[i];
    }

    ei->i_e2i->i_flags = ei->i_flags;
    ei->i_e2i->i_faddr = ei->i_faddr;
    ei->i_e2i->i_file_acl = ei->i_file_acl;
    ei->i_e2i->i_dir_acl = ei->i_dir_acl;
    ei->i_e2i->i_dtime = ei->i_dtime;

    /* Copy the inode to the buffer */
    memcpy((u8 *)buffer + offset, ei->i_e2i, sizeof(ext2_inode_t));

    /* Write the block */
    ret = ext2_write_block(sbi, block, buffer);

    /* Free the buffer */
    kfree(buffer);

    return ret;
}

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
