/**
 * util.c - Ext2 utility functions
 *
 * This file contains utility functions for the Ext2 file system.
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
 * Get the physical block number for a logical block
 * 
 * @param inode Inode
 * @param block Logical block number
 * @return Physical block number, or 0 if not allocated
 */
u32 ext2_get_block(struct inode *inode, u32 block) {
    /* Get the Ext2 inode info */
    ext2_inode_info_t *ei = (ext2_inode_info_t *)inode->fs_data;
    
    /* Get the superblock */
    super_block_t *sb = inode->i_ops->get_super(inode);
    
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Calculate the block size */
    u32 block_size = sbi->s_block_size;
    
    /* Calculate the number of blocks per indirect block */
    u32 blocks_per_indirect = block_size / sizeof(u32);
    
    /* Calculate the number of blocks per double indirect block */
    u32 blocks_per_double_indirect = blocks_per_indirect * blocks_per_indirect;
    
    /* Calculate the number of blocks per triple indirect block */
    u32 blocks_per_triple_indirect = blocks_per_indirect * blocks_per_double_indirect;
    
    /* Check if the block is a direct block */
    if (block < 12) {
        return ei->i_data[block];
    }
    
    /* Check if the block is in the indirect block */
    block -= 12;
    
    if (block < blocks_per_indirect) {
        /* Get the indirect block */
        u32 indirect_block = ei->i_data[12];
        
        if (indirect_block == 0) {
            return 0;
        }
        
        /* Allocate memory for the indirect block */
        u32 *indirect_buffer = kmalloc(block_size, 0);
        
        if (indirect_buffer == NULL) {
            printk(KERN_ERR "EXT2: Failed to allocate memory for indirect block\n");
            return 0;
        }
        
        /* Read the indirect block */
        int ret = ext2_read_block(sbi, indirect_block, indirect_buffer);
        
        if (ret < 0) {
            kfree(indirect_buffer);
            return 0;
        }
        
        /* Get the physical block number */
        u32 phys_block = indirect_buffer[block];
        
        /* Free the indirect buffer */
        kfree(indirect_buffer);
        
        return phys_block;
    }
    
    /* Check if the block is in the double indirect block */
    block -= blocks_per_indirect;
    
    if (block < blocks_per_double_indirect) {
        /* Get the double indirect block */
        u32 double_indirect_block = ei->i_data[13];
        
        if (double_indirect_block == 0) {
            return 0;
        }
        
        /* Calculate the indices */
        u32 indirect_index = block / blocks_per_indirect;
        u32 block_index = block % blocks_per_indirect;
        
        /* Allocate memory for the double indirect block */
        u32 *double_indirect_buffer = kmalloc(block_size, 0);
        
        if (double_indirect_buffer == NULL) {
            printk(KERN_ERR "EXT2: Failed to allocate memory for double indirect block\n");
            return 0;
        }
        
        /* Read the double indirect block */
        int ret = ext2_read_block(sbi, double_indirect_block, double_indirect_buffer);
        
        if (ret < 0) {
            kfree(double_indirect_buffer);
            return 0;
        }
        
        /* Get the indirect block */
        u32 indirect_block = double_indirect_buffer[indirect_index];
        
        /* Free the double indirect buffer */
        kfree(double_indirect_buffer);
        
        if (indirect_block == 0) {
            return 0;
        }
        
        /* Allocate memory for the indirect block */
        u32 *indirect_buffer = kmalloc(block_size, 0);
        
        if (indirect_buffer == NULL) {
            printk(KERN_ERR "EXT2: Failed to allocate memory for indirect block\n");
            return 0;
        }
        
        /* Read the indirect block */
        ret = ext2_read_block(sbi, indirect_block, indirect_buffer);
        
        if (ret < 0) {
            kfree(indirect_buffer);
            return 0;
        }
        
        /* Get the physical block number */
        u32 phys_block = indirect_buffer[block_index];
        
        /* Free the indirect buffer */
        kfree(indirect_buffer);
        
        return phys_block;
    }
    
    /* Check if the block is in the triple indirect block */
    block -= blocks_per_double_indirect;
    
    if (block < blocks_per_triple_indirect) {
        /* Get the triple indirect block */
        u32 triple_indirect_block = ei->i_data[14];
        
        if (triple_indirect_block == 0) {
            return 0;
        }
        
        /* Calculate the indices */
        u32 double_indirect_index = block / blocks_per_double_indirect;
        u32 indirect_index = (block % blocks_per_double_indirect) / blocks_per_indirect;
        u32 block_index = block % blocks_per_indirect;
        
        /* Allocate memory for the triple indirect block */
        u32 *triple_indirect_buffer = kmalloc(block_size, 0);
        
        if (triple_indirect_buffer == NULL) {
            printk(KERN_ERR "EXT2: Failed to allocate memory for triple indirect block\n");
            return 0;
        }
        
        /* Read the triple indirect block */
        int ret = ext2_read_block(sbi, triple_indirect_block, triple_indirect_buffer);
        
        if (ret < 0) {
            kfree(triple_indirect_buffer);
            return 0;
        }
        
        /* Get the double indirect block */
        u32 double_indirect_block = triple_indirect_buffer[double_indirect_index];
        
        /* Free the triple indirect buffer */
        kfree(triple_indirect_buffer);
        
        if (double_indirect_block == 0) {
            return 0;
        }
        
        /* Allocate memory for the double indirect block */
        u32 *double_indirect_buffer = kmalloc(block_size, 0);
        
        if (double_indirect_buffer == NULL) {
            printk(KERN_ERR "EXT2: Failed to allocate memory for double indirect block\n");
            return 0;
        }
        
        /* Read the double indirect block */
        ret = ext2_read_block(sbi, double_indirect_block, double_indirect_buffer);
        
        if (ret < 0) {
            kfree(double_indirect_buffer);
            return 0;
        }
        
        /* Get the indirect block */
        u32 indirect_block = double_indirect_buffer[indirect_index];
        
        /* Free the double indirect buffer */
        kfree(double_indirect_buffer);
        
        if (indirect_block == 0) {
            return 0;
        }
        
        /* Allocate memory for the indirect block */
        u32 *indirect_buffer = kmalloc(block_size, 0);
        
        if (indirect_buffer == NULL) {
            printk(KERN_ERR "EXT2: Failed to allocate memory for indirect block\n");
            return 0;
        }
        
        /* Read the indirect block */
        ret = ext2_read_block(sbi, indirect_block, indirect_buffer);
        
        if (ret < 0) {
            kfree(indirect_buffer);
            return 0;
        }
        
        /* Get the physical block number */
        u32 phys_block = indirect_buffer[block_index];
        
        /* Free the indirect buffer */
        kfree(indirect_buffer);
        
        return phys_block;
    }
    
    /* Block number is too large */
    return 0;
}

/**
 * Allocate a block
 * 
 * @param inode Inode
 * @param block Logical block number
 * @return Physical block number, or 0 on failure
 */
u32 ext2_alloc_block(struct inode *inode, u32 block) {
    /* Get the Ext2 inode info */
    ext2_inode_info_t *ei = (ext2_inode_info_t *)inode->fs_data;
    
    /* Get the superblock */
    super_block_t *sb = inode->i_ops->get_super(inode);
    
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Calculate the block size */
    u32 block_size = sbi->s_block_size;
    
    /* Calculate the number of blocks per indirect block */
    u32 blocks_per_indirect = block_size / sizeof(u32);
    
    /* Calculate the number of blocks per double indirect block */
    u32 blocks_per_double_indirect = blocks_per_indirect * blocks_per_indirect;
    
    /* Calculate the number of blocks per triple indirect block */
    u32 blocks_per_triple_indirect = blocks_per_indirect * blocks_per_double_indirect;
    
    /* Allocate a new block */
    u32 phys_block = ext2_new_block(inode);
    
    if (phys_block == 0) {
        return 0;
    }
    
    /* Check if the block is a direct block */
    if (block < 12) {
        ei->i_data[block] = phys_block;
        return phys_block;
    }
    
    /* Check if the block is in the indirect block */
    block -= 12;
    
    if (block < blocks_per_indirect) {
        /* Get the indirect block */
        u32 indirect_block = ei->i_data[12];
        
        if (indirect_block == 0) {
            /* Allocate a new indirect block */
            indirect_block = ext2_new_block(inode);
            
            if (indirect_block == 0) {
                ext2_free_block(inode, phys_block);
                return 0;
            }
            
            /* Set the indirect block */
            ei->i_data[12] = indirect_block;
            
            /* Clear the indirect block */
            u32 *indirect_buffer = kmalloc(block_size, 0);
            
            if (indirect_buffer == NULL) {
                printk(KERN_ERR "EXT2: Failed to allocate memory for indirect block\n");
                ext2_free_block(inode, phys_block);
                ext2_free_block(inode, indirect_block);
                return 0;
            }
            
            memset(indirect_buffer, 0, block_size);
            
            /* Write the indirect block */
            int ret = ext2_write_block(sbi, indirect_block, indirect_buffer);
            
            if (ret < 0) {
                kfree(indirect_buffer);
                ext2_free_block(inode, phys_block);
                ext2_free_block(inode, indirect_block);
                return 0;
            }
            
            /* Free the indirect buffer */
            kfree(indirect_buffer);
        }
        
        /* Allocate memory for the indirect block */
        u32 *indirect_buffer = kmalloc(block_size, 0);
        
        if (indirect_buffer == NULL) {
            printk(KERN_ERR "EXT2: Failed to allocate memory for indirect block\n");
            ext2_free_block(inode, phys_block);
            return 0;
        }
        
        /* Read the indirect block */
        int ret = ext2_read_block(sbi, indirect_block, indirect_buffer);
        
        if (ret < 0) {
            kfree(indirect_buffer);
            ext2_free_block(inode, phys_block);
            return 0;
        }
        
        /* Set the physical block number */
        indirect_buffer[block] = phys_block;
        
        /* Write the indirect block */
        ret = ext2_write_block(sbi, indirect_block, indirect_buffer);
        
        if (ret < 0) {
            kfree(indirect_buffer);
            ext2_free_block(inode, phys_block);
            return 0;
        }
        
        /* Free the indirect buffer */
        kfree(indirect_buffer);
        
        return phys_block;
    }
    
    /* Block is in the double or triple indirect block */
    /* This is left as an exercise for the reader */
    
    /* Free the block */
    ext2_free_block(inode, phys_block);
    
    return 0;
}

/**
 * Allocate a new block
 * 
 * @param inode Inode
 * @return Physical block number, or 0 on failure
 */
u32 ext2_new_block(struct inode *inode) {
    /* Get the superblock */
    super_block_t *sb = inode->i_ops->get_super(inode);
    
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Calculate the block size */
    u32 block_size = sbi->s_block_size;
    
    /* Check if there are any free blocks */
    if (sbi->s_es->s_free_blocks_count == 0) {
        printk(KERN_ERR "EXT2: No free blocks\n");
        return 0;
    }
    
    /* Find a block group with free blocks */
    for (u32 i = 0; i < sbi->s_groups_count; i++) {
        /* Get the block group */
        ext2_group_desc_t *group = &sbi->s_group_desc[i];
        
        /* Check if there are any free blocks */
        if (group->bg_free_blocks_count == 0) {
            continue;
        }
        
        /* Allocate memory for the block bitmap */
        u8 *bitmap = kmalloc(block_size, 0);
        
        if (bitmap == NULL) {
            printk(KERN_ERR "EXT2: Failed to allocate memory for block bitmap\n");
            return 0;
        }
        
        /* Read the block bitmap */
        int ret = ext2_read_block(sbi, group->bg_block_bitmap, bitmap);
        
        if (ret < 0) {
            kfree(bitmap);
            return 0;
        }
        
        /* Find a free block */
        for (u32 j = 0; j < sbi->s_blocks_per_group; j++) {
            /* Check if the block is free */
            if (!(bitmap[j / 8] & (1 << (j % 8)))) {
                /* Mark the block as used */
                bitmap[j / 8] |= (1 << (j % 8));
                
                /* Write the block bitmap */
                ret = ext2_write_block(sbi, group->bg_block_bitmap, bitmap);
                
                if (ret < 0) {
                    kfree(bitmap);
                    return 0;
                }
                
                /* Free the bitmap */
                kfree(bitmap);
                
                /* Update the group descriptor */
                group->bg_free_blocks_count--;
                
                /* Update the superblock */
                sbi->s_es->s_free_blocks_count--;
                
                /* Write the superblock */
                ret = ext2_write_super(sb);
                
                if (ret < 0) {
                    return 0;
                }
                
                /* Calculate the physical block number */
                u32 phys_block = i * sbi->s_blocks_per_group + j + sbi->s_first_data_block;
                
                return phys_block;
            }
        }
        
        /* Free the bitmap */
        kfree(bitmap);
    }
    
    /* No free blocks found */
    return 0;
}

/**
 * Free a block
 * 
 * @param inode Inode
 * @param block Physical block number
 * @return 0 on success, negative error code on failure
 */
int ext2_free_block(struct inode *inode, u32 block) {
    /* Get the superblock */
    super_block_t *sb = inode->i_ops->get_super(inode);
    
    /* Get the Ext2 superblock info */
    ext2_sb_info_t *sbi = (ext2_sb_info_t *)sb->fs_data;
    
    /* Calculate the block size */
    u32 block_size = sbi->s_block_size;
    
    /* Calculate the block group */
    u32 group = (block - sbi->s_first_data_block) / sbi->s_blocks_per_group;
    
    /* Calculate the block index within the group */
    u32 index = (block - sbi->s_first_data_block) % sbi->s_blocks_per_group;
    
    /* Get the block group */
    ext2_group_desc_t *group_desc = &sbi->s_group_desc[group];
    
    /* Allocate memory for the block bitmap */
    u8 *bitmap = kmalloc(block_size, 0);
    
    if (bitmap == NULL) {
        printk(KERN_ERR "EXT2: Failed to allocate memory for block bitmap\n");
        return -ENOMEM;
    }
    
    /* Read the block bitmap */
    int ret = ext2_read_block(sbi, group_desc->bg_block_bitmap, bitmap);
    
    if (ret < 0) {
        kfree(bitmap);
        return ret;
    }
    
    /* Check if the block is already free */
    if (!(bitmap[index / 8] & (1 << (index % 8)))) {
        kfree(bitmap);
        return 0;
    }
    
    /* Mark the block as free */
    bitmap[index / 8] &= ~(1 << (index % 8));
    
    /* Write the block bitmap */
    ret = ext2_write_block(sbi, group_desc->bg_block_bitmap, bitmap);
    
    if (ret < 0) {
        kfree(bitmap);
        return ret;
    }
    
    /* Free the bitmap */
    kfree(bitmap);
    
    /* Update the group descriptor */
    group_desc->bg_free_blocks_count++;
    
    /* Update the superblock */
    sbi->s_es->s_free_blocks_count++;
    
    /* Write the superblock */
    ret = ext2_write_super(sb);
    
    return ret;
}
