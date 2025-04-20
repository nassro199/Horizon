/**
 * inode.c - Horizon kernel RAM file system inode implementation
 * 
 * This file contains the implementation of the RAM file system inode.
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

/* RAM file system directory inode operations */
struct inode_operations ramfs_dir_inode_ops = {
    .lookup = ramfs_lookup,
    .create = ramfs_create,
    .link = ramfs_link,
    .unlink = ramfs_unlink,
    .symlink = ramfs_symlink,
    .mkdir = ramfs_mkdir,
    .rmdir = ramfs_rmdir,
    .mknod = ramfs_mknod,
    .rename = ramfs_rename
};

/* RAM file system file inode operations */
struct inode_operations ramfs_file_inode_ops = {
    .getattr = ramfs_getattr,
    .setattr = ramfs_setattr
};

/* RAM file system lookup */
struct dentry *ramfs_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags) {
    if (dir == NULL || dentry == NULL) {
        return NULL;
    }
    
    /* Get the directory entry */
    struct ramfs_dirent *dirent = ramfs_find_dirent(dir, dentry->d_name.name, dentry->d_name.len);
    
    if (dirent == NULL) {
        return NULL;
    }
    
    /* Get the inode */
    struct inode *inode = ramfs_get_inode(dir->i_sb, dir, dirent->mode, 0);
    
    if (inode == NULL) {
        return NULL;
    }
    
    /* Set the inode number */
    inode->i_ino = dirent->ino;
    
    /* Add the dentry */
    d_add(dentry, inode);
    
    return NULL;
}

/* RAM file system create */
int ramfs_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl) {
    if (dir == NULL || dentry == NULL) {
        return -1;
    }
    
    /* Create the inode */
    struct inode *inode = ramfs_get_inode(dir->i_sb, dir, mode, 0);
    
    if (inode == NULL) {
        return -1;
    }
    
    /* Add the directory entry */
    int error = ramfs_add_dirent(dir, dentry->d_name.name, dentry->d_name.len, inode);
    
    if (error) {
        iput(inode);
        return error;
    }
    
    /* Add the dentry */
    d_instantiate(dentry, inode);
    
    return 0;
}

/* RAM file system link */
int ramfs_link(struct dentry *old_dentry, struct inode *dir, struct dentry *dentry) {
    if (old_dentry == NULL || dir == NULL || dentry == NULL) {
        return -1;
    }
    
    /* Get the inode */
    struct inode *inode = old_dentry->d_inode;
    
    if (inode == NULL) {
        return -1;
    }
    
    /* Increment the link count */
    inode->i_nlink++;
    
    /* Add the directory entry */
    int error = ramfs_add_dirent(dir, dentry->d_name.name, dentry->d_name.len, inode);
    
    if (error) {
        inode->i_nlink--;
        return error;
    }
    
    /* Add the dentry */
    d_instantiate(dentry, inode);
    
    return 0;
}

/* RAM file system unlink */
int ramfs_unlink(struct inode *dir, struct dentry *dentry) {
    if (dir == NULL || dentry == NULL) {
        return -1;
    }
    
    /* Get the inode */
    struct inode *inode = dentry->d_inode;
    
    if (inode == NULL) {
        return -1;
    }
    
    /* Remove the directory entry */
    int error = ramfs_remove_dirent(dir, dentry->d_name.name, dentry->d_name.len);
    
    if (error) {
        return error;
    }
    
    /* Decrement the link count */
    inode->i_nlink--;
    
    return 0;
}

/* RAM file system symlink */
int ramfs_symlink(struct inode *dir, struct dentry *dentry, const char *symname) {
    if (dir == NULL || dentry == NULL || symname == NULL) {
        return -1;
    }
    
    /* Create the inode */
    struct inode *inode = ramfs_get_inode(dir->i_sb, dir, S_IFLNK | 0777, 0);
    
    if (inode == NULL) {
        return -1;
    }
    
    /* Set the symlink */
    int error = ramfs_set_link(inode, symname);
    
    if (error) {
        iput(inode);
        return error;
    }
    
    /* Add the directory entry */
    error = ramfs_add_dirent(dir, dentry->d_name.name, dentry->d_name.len, inode);
    
    if (error) {
        iput(inode);
        return error;
    }
    
    /* Add the dentry */
    d_instantiate(dentry, inode);
    
    return 0;
}

/* RAM file system mkdir */
int ramfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode) {
    if (dir == NULL || dentry == NULL) {
        return -1;
    }
    
    /* Create the inode */
    struct inode *inode = ramfs_get_inode(dir->i_sb, dir, S_IFDIR | mode, 0);
    
    if (inode == NULL) {
        return -1;
    }
    
    /* Add the directory entry */
    int error = ramfs_add_dirent(dir, dentry->d_name.name, dentry->d_name.len, inode);
    
    if (error) {
        iput(inode);
        return error;
    }
    
    /* Add the dentry */
    d_instantiate(dentry, inode);
    
    return 0;
}

/* RAM file system rmdir */
int ramfs_rmdir(struct inode *dir, struct dentry *dentry) {
    if (dir == NULL || dentry == NULL) {
        return -1;
    }
    
    /* Get the inode */
    struct inode *inode = dentry->d_inode;
    
    if (inode == NULL) {
        return -1;
    }
    
    /* Check if the directory is empty */
    if (!ramfs_empty_dir(inode)) {
        return -1;
    }
    
    /* Remove the directory entry */
    int error = ramfs_remove_dirent(dir, dentry->d_name.name, dentry->d_name.len);
    
    if (error) {
        return error;
    }
    
    /* Decrement the link count */
    inode->i_nlink--;
    
    return 0;
}

/* RAM file system mknod */
int ramfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev) {
    if (dir == NULL || dentry == NULL) {
        return -1;
    }
    
    /* Create the inode */
    struct inode *inode = ramfs_get_inode(dir->i_sb, dir, mode, dev);
    
    if (inode == NULL) {
        return -1;
    }
    
    /* Add the directory entry */
    int error = ramfs_add_dirent(dir, dentry->d_name.name, dentry->d_name.len, inode);
    
    if (error) {
        iput(inode);
        return error;
    }
    
    /* Add the dentry */
    d_instantiate(dentry, inode);
    
    return 0;
}

/* RAM file system rename */
int ramfs_rename(struct inode *old_dir, struct dentry *old_dentry, struct inode *new_dir, struct dentry *new_dentry, unsigned int flags) {
    if (old_dir == NULL || old_dentry == NULL || new_dir == NULL || new_dentry == NULL) {
        return -1;
    }
    
    /* Get the inode */
    struct inode *inode = old_dentry->d_inode;
    
    if (inode == NULL) {
        return -1;
    }
    
    /* Remove the old directory entry */
    int error = ramfs_remove_dirent(old_dir, old_dentry->d_name.name, old_dentry->d_name.len);
    
    if (error) {
        return error;
    }
    
    /* Add the new directory entry */
    error = ramfs_add_dirent(new_dir, new_dentry->d_name.name, new_dentry->d_name.len, inode);
    
    if (error) {
        /* Add the old directory entry back */
        ramfs_add_dirent(old_dir, old_dentry->d_name.name, old_dentry->d_name.len, inode);
        return error;
    }
    
    return 0;
}

/* RAM file system getattr */
int ramfs_getattr(const struct path *path, struct kstat *stat, u32 request_mask, unsigned int flags) {
    if (path == NULL || stat == NULL) {
        return -1;
    }
    
    /* Get the inode */
    struct inode *inode = path->dentry->d_inode;
    
    if (inode == NULL) {
        return -1;
    }
    
    /* Get the RAM file system inode */
    struct ramfs_inode *ramfs_inode = container_of(inode, struct ramfs_inode, vfs_inode);
    
    /* Set the statistics */
    stat->dev = inode->i_sb->s_dev;
    stat->ino = inode->i_ino;
    stat->mode = inode->i_mode;
    stat->nlink = inode->i_nlink;
    stat->uid = inode->i_uid;
    stat->gid = inode->i_gid;
    stat->rdev = inode->i_rdev;
    stat->size = ramfs_inode->size;
    stat->atime = inode->i_atime;
    stat->mtime = inode->i_mtime;
    stat->ctime = inode->i_ctime;
    stat->blksize = PAGE_SIZE;
    stat->blocks = (ramfs_inode->size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    return 0;
}

/* RAM file system setattr */
int ramfs_setattr(struct dentry *dentry, struct iattr *attr) {
    if (dentry == NULL || attr == NULL) {
        return -1;
    }
    
    /* Get the inode */
    struct inode *inode = dentry->d_inode;
    
    if (inode == NULL) {
        return -1;
    }
    
    /* Get the RAM file system inode */
    struct ramfs_inode *ramfs_inode = container_of(inode, struct ramfs_inode, vfs_inode);
    
    /* Set the attributes */
    if (attr->ia_valid & ATTR_SIZE) {
        /* Truncate the file */
        if (attr->ia_size < ramfs_inode->size) {
            /* Shrink the file */
            ramfs_inode->size = attr->ia_size;
        } else if (attr->ia_size > ramfs_inode->size) {
            /* Grow the file */
            void *data = krealloc(ramfs_inode->data, attr->ia_size, MEM_KERNEL);
            
            if (data == NULL) {
                return -1;
            }
            
            /* Zero the new data */
            memset((char *)data + ramfs_inode->size, 0, attr->ia_size - ramfs_inode->size);
            
            /* Set the new data */
            ramfs_inode->data = data;
            ramfs_inode->size = attr->ia_size;
        }
        
        /* Update the inode size */
        inode->i_size = ramfs_inode->size;
    }
    
    if (attr->ia_valid & ATTR_MODE) {
        /* Set the mode */
        inode->i_mode = attr->ia_mode;
    }
    
    if (attr->ia_valid & ATTR_UID) {
        /* Set the user ID */
        inode->i_uid = attr->ia_uid;
    }
    
    if (attr->ia_valid & ATTR_GID) {
        /* Set the group ID */
        inode->i_gid = attr->ia_gid;
    }
    
    if (attr->ia_valid & ATTR_ATIME) {
        /* Set the access time */
        inode->i_atime = attr->ia_atime;
    }
    
    if (attr->ia_valid & ATTR_MTIME) {
        /* Set the modification time */
        inode->i_mtime = attr->ia_mtime;
    }
    
    if (attr->ia_valid & ATTR_CTIME) {
        /* Set the change time */
        inode->i_ctime = attr->ia_ctime;
    }
    
    return 0;
}
