/**
 * path.h - Path definitions
 *
 * This file contains definitions for paths.
 */

#ifndef _HORIZON_FS_PATH_H
#define _HORIZON_FS_PATH_H

#include <horizon/types.h>

/* Forward declarations */
struct vfsmount;
struct dentry;
struct inode;

/* Path structure */
struct path {
    struct vfsmount *mnt;    /* Mount point */
    struct dentry *dentry;   /* Dentry */
};

/* Dentry structure */
struct dentry {
    struct inode *d_inode;   /* Inode */
    char *d_name;            /* Name */
    struct dentry *d_parent; /* Parent dentry */
    struct list_head d_subdirs; /* Subdirectories */
    struct list_head d_child;   /* Child list */
    struct list_head d_alias;   /* Alias list */
    unsigned int d_flags;    /* Flags */
    void *d_fsdata;          /* File system specific data */
};

#endif /* _HORIZON_FS_PATH_H */
