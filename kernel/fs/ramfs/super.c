/**
 * super.c - Horizon kernel RAM file system superblock implementation
 * 
 * This file contains the implementation of the RAM file system superblock.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/fs/vfs.h>
#include <horizon/fs/ramfs/ramfs.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* RAM file system type */
struct file_system_type ramfs_fs_type = {
    .name = "ramfs",
    .fs_flags = 0,
    .mount = ramfs_mount,
    .kill_sb = ramfs_kill_sb,
    .owner = NULL,
    .next = NULL
};

/* RAM file system superblock operations */
struct super_operations ramfs_super_ops = {
    .alloc_inode = ramfs_alloc_inode,
    .destroy_inode = ramfs_destroy_inode,
    .write_inode = ramfs_write_inode,
    .put_super = ramfs_put_super,
    .statfs = ramfs_statfs
};

/* Initialize the RAM file system */
int ramfs_init(void) {
    /* Register the RAM file system */
    return register_filesystem(&ramfs_fs_type);
}

/* Mount a RAM file system */
struct dentry *ramfs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data) {
    /* Create a superblock */
    struct super_block *sb = kmalloc(sizeof(struct super_block), MEM_KERNEL | MEM_ZERO);
    
    if (sb == NULL) {
        return NULL;
    }
    
    /* Initialize the superblock */
    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_magic = RAMFS_MAGIC;
    sb->s_op = &ramfs_super_ops;
    sb->s_type = fs_type;
    
    /* Create the root inode */
    struct inode *root_inode = ramfs_get_inode(sb, NULL, S_IFDIR | 0755, 0);
    
    if (root_inode == NULL) {
        kfree(sb);
        return NULL;
    }
    
    /* Create the root dentry */
    struct dentry *root_dentry = d_alloc_root(root_inode);
    
    if (root_dentry == NULL) {
        iput(root_inode);
        kfree(sb);
        return NULL;
    }
    
    /* Set the superblock root */
    sb->s_root = root_dentry;
    
    return root_dentry;
}

/* Kill a RAM file system superblock */
void ramfs_kill_sb(struct super_block *sb) {
    if (sb == NULL) {
        return;
    }
    
    /* Free the superblock */
    kfree(sb);
}

/* Allocate a RAM file system inode */
struct inode *ramfs_alloc_inode(struct super_block *sb) {
    /* Allocate a RAM file system inode */
    struct ramfs_inode *inode = kmalloc(sizeof(struct ramfs_inode), MEM_KERNEL | MEM_ZERO);
    
    if (inode == NULL) {
        return NULL;
    }
    
    /* Initialize the inode */
    inode->data = NULL;
    inode->size = 0;
    
    return &inode->vfs_inode;
}

/* Destroy a RAM file system inode */
void ramfs_destroy_inode(struct inode *inode) {
    if (inode == NULL) {
        return;
    }
    
    /* Get the RAM file system inode */
    struct ramfs_inode *ramfs_inode = container_of(inode, struct ramfs_inode, vfs_inode);
    
    /* Free the file data */
    if (ramfs_inode->data != NULL) {
        kfree(ramfs_inode->data);
    }
    
    /* Free the inode */
    kfree(ramfs_inode);
}

/* Write a RAM file system inode */
int ramfs_write_inode(struct inode *inode, struct writeback_control *wbc) {
    /* RAM file system inodes are always up to date */
    return 0;
}

/* Put a RAM file system superblock */
void ramfs_put_super(struct super_block *sb) {
    /* Nothing to do */
}

/* Get RAM file system statistics */
int ramfs_statfs(struct dentry *dentry, struct kstatfs *buf) {
    if (buf == NULL) {
        return -1;
    }
    
    /* Set the statistics */
    buf->f_type = RAMFS_MAGIC;
    buf->f_bsize = PAGE_SIZE;
    buf->f_namelen = NAME_MAX;
    
    return 0;
}

/* Get a RAM file system inode */
struct inode *ramfs_get_inode(struct super_block *sb, struct inode *dir, umode_t mode, dev_t dev) {
    if (sb == NULL) {
        return NULL;
    }
    
    /* Allocate an inode */
    struct inode *inode = new_inode(sb);
    
    if (inode == NULL) {
        return NULL;
    }
    
    /* Initialize the inode */
    inode->i_mode = mode;
    inode->i_uid = 0;
    inode->i_gid = 0;
    inode->i_blocks = 0;
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time();
    
    /* Set the inode operations */
    if (S_ISDIR(mode)) {
        inode->i_op = &ramfs_dir_inode_ops;
        inode->i_fop = &ramfs_dir_ops;
    } else if (S_ISREG(mode)) {
        inode->i_op = &ramfs_file_inode_ops;
        inode->i_fop = &ramfs_file_ops;
    } else {
        init_special_inode(inode, mode, dev);
    }
    
    return inode;
}
