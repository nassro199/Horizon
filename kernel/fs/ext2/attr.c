/**
 * attr.c - Ext2 attribute operations
 *
 * This file contains the implementation of Ext2 attribute operations.
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
 * Set attributes of an inode
 * 
 * @param inode Inode to set attributes for
 * @param attr Attributes to set
 * @return 0 on success, negative error code on failure
 */
error_t ext2_setattr(struct inode *inode, struct iattr *attr) {
    /* Check if the inode exists */
    if (inode == NULL) {
        printk(KERN_ERR "EXT2: Inode does not exist\n");
        return -ENOENT;
    }
    
    /* Check if the attributes are valid */
    if (attr == NULL) {
        printk(KERN_ERR "EXT2: Invalid attributes\n");
        return -EINVAL;
    }
    
    /* Get the Ext2 inode info */
    ext2_inode_info_t *ei = (ext2_inode_info_t *)inode->fs_data;
    
    /* Get the superblock */
    super_block_t *sb = inode->i_ops->get_super(inode);
    
    /* Check if the file system is read-only */
    if (sb->flags & MOUNT_READ_ONLY) {
        printk(KERN_ERR "EXT2: File system is read-only\n");
        return -EROFS;
    }
    
    /* Set the attributes */
    if (attr->ia_valid & ATTR_MODE) {
        inode->permissions = attr->ia_mode & 0xFFF;
        ei->i_e2i->i_mode = (ei->i_e2i->i_mode & ~0xFFF) | inode->permissions;
    }
    
    if (attr->ia_valid & ATTR_UID) {
        inode->uid = attr->ia_uid;
        ei->i_e2i->i_uid = inode->uid;
    }
    
    if (attr->ia_valid & ATTR_GID) {
        inode->gid = attr->ia_gid;
        ei->i_e2i->i_gid = inode->gid;
    }
    
    if (attr->ia_valid & ATTR_SIZE) {
        /* Check if the size is changing */
        if (attr->ia_size != inode->size) {
            /* Truncate the file */
            error_t ret = ext2_truncate(inode, attr->ia_size);
            
            if (ret < 0) {
                return ret;
            }
        }
    }
    
    if (attr->ia_valid & ATTR_ATIME) {
        inode->atime = attr->ia_atime;
        ei->i_e2i->i_atime = inode->atime;
    }
    
    if (attr->ia_valid & ATTR_MTIME) {
        inode->mtime = attr->ia_mtime;
        ei->i_e2i->i_mtime = inode->mtime;
    }
    
    if (attr->ia_valid & ATTR_CTIME) {
        inode->ctime = attr->ia_ctime;
        ei->i_e2i->i_ctime = inode->ctime;
    }
    
    /* Write the inode */
    return ext2_write_inode(sb, inode);
}

/**
 * Get attributes of an inode
 * 
 * @param inode Inode to get attributes for
 * @param attr Attributes to fill
 * @return 0 on success, negative error code on failure
 */
error_t ext2_getattr(struct inode *inode, struct iattr *attr) {
    /* Check if the inode exists */
    if (inode == NULL) {
        printk(KERN_ERR "EXT2: Inode does not exist\n");
        return -ENOENT;
    }
    
    /* Check if the attributes are valid */
    if (attr == NULL) {
        printk(KERN_ERR "EXT2: Invalid attributes\n");
        return -EINVAL;
    }
    
    /* Get the attributes */
    attr->ia_mode = inode->permissions;
    attr->ia_uid = inode->uid;
    attr->ia_gid = inode->gid;
    attr->ia_size = inode->size;
    attr->ia_atime = inode->atime;
    attr->ia_mtime = inode->mtime;
    attr->ia_ctime = inode->ctime;
    
    /* Set the valid flags */
    attr->ia_valid = ATTR_MODE | ATTR_UID | ATTR_GID | ATTR_SIZE | ATTR_ATIME | ATTR_MTIME | ATTR_CTIME;
    
    return 0;
}

/**
 * Check permissions for an inode
 * 
 * @param inode Inode to check permissions for
 * @param mask Permission mask to check
 * @return 0 on success, negative error code on failure
 */
error_t ext2_permission(struct inode *inode, int mask) {
    /* Check if the inode exists */
    if (inode == NULL) {
        printk(KERN_ERR "EXT2: Inode does not exist\n");
        return -ENOENT;
    }
    
    /* Get the current user ID */
    u32 uid = 0; /* Root user */
    
    /* Get the current group ID */
    u32 gid = 0; /* Root group */
    
    /* Check if the user is the owner */
    if (uid == inode->uid) {
        /* Check owner permissions */
        if ((mask & FILE_PERM_READ) && !(inode->permissions & FILE_PERM_USER_READ)) {
            return -EACCES;
        }
        
        if ((mask & FILE_PERM_WRITE) && !(inode->permissions & FILE_PERM_USER_WRITE)) {
            return -EACCES;
        }
        
        if ((mask & FILE_PERM_EXEC) && !(inode->permissions & FILE_PERM_USER_EXEC)) {
            return -EACCES;
        }
    } else if (gid == inode->gid) {
        /* Check group permissions */
        if ((mask & FILE_PERM_READ) && !(inode->permissions & FILE_PERM_GROUP_READ)) {
            return -EACCES;
        }
        
        if ((mask & FILE_PERM_WRITE) && !(inode->permissions & FILE_PERM_GROUP_WRITE)) {
            return -EACCES;
        }
        
        if ((mask & FILE_PERM_EXEC) && !(inode->permissions & FILE_PERM_GROUP_EXEC)) {
            return -EACCES;
        }
    } else {
        /* Check other permissions */
        if ((mask & FILE_PERM_READ) && !(inode->permissions & FILE_PERM_OTHER_READ)) {
            return -EACCES;
        }
        
        if ((mask & FILE_PERM_WRITE) && !(inode->permissions & FILE_PERM_OTHER_WRITE)) {
            return -EACCES;
        }
        
        if ((mask & FILE_PERM_EXEC) && !(inode->permissions & FILE_PERM_OTHER_EXEC)) {
            return -EACCES;
        }
    }
    
    return 0;
}

/**
 * Truncate an inode
 * 
 * @param inode Inode to truncate
 * @param size New size
 * @return 0 on success, negative error code on failure
 */
error_t ext2_truncate(struct inode *inode, u64 size) {
    /* Check if the inode exists */
    if (inode == NULL) {
        printk(KERN_ERR "EXT2: Inode does not exist\n");
        return -ENOENT;
    }
    
    /* Get the Ext2 inode info */
    ext2_inode_info_t *ei = (ext2_inode_info_t *)inode->fs_data;
    
    /* Get the superblock */
    super_block_t *sb = inode->i_ops->get_super(inode);
    
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Check if the file system is read-only */
    if (sb->flags & MOUNT_READ_ONLY) {
        printk(KERN_ERR "EXT2: File system is read-only\n");
        return -EROFS;
    }
    
    /* Check if the size is changing */
    if (size == inode->size) {
        return 0;
    }
    
    /* Calculate the block size */
    u32 block_size = sbi->s_block_size;
    
    /* Calculate the old and new block counts */
    u32 old_blocks = (inode->size + block_size - 1) / block_size;
    u32 new_blocks = (size + block_size - 1) / block_size;
    
    /* Check if we need to free blocks */
    if (new_blocks < old_blocks) {
        /* Free the blocks */
        for (u32 i = new_blocks; i < old_blocks; i++) {
            /* Get the physical block number */
            u32 phys_block = ext2_get_block(inode, i);
            
            if (phys_block != 0) {
                /* Free the block */
                ext2_free_block(inode, phys_block);
                
                /* Clear the block pointer */
                if (i < 12) {
                    ei->i_data[i] = 0;
                    ei->i_e2i->i_block[i] = 0;
                } else {
                    /* Indirect blocks are handled by ext2_free_block */
                }
            }
        }
    }
    
    /* Set the new size */
    inode->size = size;
    ei->i_e2i->i_size = size;
    
    /* Calculate the new block count */
    u32 blocks = 0;
    
    for (u32 i = 0; i < new_blocks; i++) {
        /* Get the physical block number */
        u32 phys_block = ext2_get_block(inode, i);
        
        if (phys_block != 0) {
            blocks++;
        }
    }
    
    /* Set the new block count */
    inode->blocks = blocks * (block_size / 512);
    ei->i_e2i->i_blocks = inode->blocks;
    
    /* Update the modification time */
    inode->mtime = time_get_unix_time();
    ei->i_e2i->i_mtime = inode->mtime;
    
    /* Update the change time */
    inode->ctime = inode->mtime;
    ei->i_e2i->i_ctime = inode->ctime;
    
    /* Write the inode */
    return ext2_write_inode(sb, inode);
}
