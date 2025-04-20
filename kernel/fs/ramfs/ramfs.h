/**
 * ramfs.h - Horizon kernel RAM file system definitions
 * 
 * This file contains definitions for the RAM file system.
 */

#ifndef _KERNEL_FS_RAMFS_H
#define _KERNEL_FS_RAMFS_H

#include <horizon/types.h>
#include <horizon/fs/vfs.h>

/* RAM file system magic number */
#define RAMFS_MAGIC 0x858458f6

/* RAM file system inode */
typedef struct ramfs_inode {
    struct inode vfs_inode;      /* VFS inode */
    void *data;                  /* File data */
    size_t size;                 /* File size */
} ramfs_inode_t;

/* RAM file system functions */
extern struct file_system_type ramfs_fs_type;
extern struct super_operations ramfs_super_ops;
extern struct inode_operations ramfs_dir_inode_ops;
extern struct inode_operations ramfs_file_inode_ops;
extern struct file_operations ramfs_dir_ops;
extern struct file_operations ramfs_file_ops;

/* RAM file system initialization */
int ramfs_init(void);

#endif /* _KERNEL_FS_RAMFS_H */
