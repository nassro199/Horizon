/**
 * dir.c - Horizon kernel RAM file system directory implementation
 * 
 * This file contains the implementation of the RAM file system directory.
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

/* RAM file system directory */
typedef struct ramfs_dir {
    int count;                   /* Number of entries */
    struct ramfs_dirent *entries; /* Directory entries */
} ramfs_dir_t;

/* RAM file system directory entry */
typedef struct ramfs_dirent {
    char *name;                  /* Entry name */
    int len;                     /* Name length */
    ino_t ino;                   /* Inode number */
    unsigned char type;          /* Entry type */
    umode_t mode;                /* Entry mode */
} ramfs_dirent_t;

/* Find a directory entry */
struct ramfs_dirent *ramfs_find_dirent(struct inode *dir, const char *name, int len) {
    if (dir == NULL || name == NULL) {
        return NULL;
    }
    
    /* Get the RAM file system inode */
    struct ramfs_inode *ramfs_inode = container_of(dir, struct ramfs_inode, vfs_inode);
    
    /* Get the directory */
    struct ramfs_dir *ramfs_dir = (struct ramfs_dir *)ramfs_inode->data;
    
    if (ramfs_dir == NULL) {
        return NULL;
    }
    
    /* Find the directory entry */
    for (int i = 0; i < ramfs_dir->count; i++) {
        /* Get the directory entry */
        struct ramfs_dirent *dirent = &ramfs_dir->entries[i];
        
        /* Check if the name matches */
        if (dirent->len == len && memcmp(dirent->name, name, len) == 0) {
            return dirent;
        }
    }
    
    return NULL;
}

/* Add a directory entry */
int ramfs_add_dirent(struct inode *dir, const char *name, int len, struct inode *inode) {
    if (dir == NULL || name == NULL || inode == NULL) {
        return -1;
    }
    
    /* Get the RAM file system inode */
    struct ramfs_inode *ramfs_inode = container_of(dir, struct ramfs_inode, vfs_inode);
    
    /* Get the directory */
    struct ramfs_dir *ramfs_dir = (struct ramfs_dir *)ramfs_inode->data;
    
    if (ramfs_dir == NULL) {
        /* Create the directory */
        ramfs_dir = kmalloc(sizeof(struct ramfs_dir), MEM_KERNEL | MEM_ZERO);
        
        if (ramfs_dir == NULL) {
            return -1;
        }
        
        /* Initialize the directory */
        ramfs_dir->count = 0;
        ramfs_dir->entries = NULL;
        
        /* Set the directory */
        ramfs_inode->data = ramfs_dir;
    }
    
    /* Check if the entry already exists */
    struct ramfs_dirent *dirent = ramfs_find_dirent(dir, name, len);
    
    if (dirent != NULL) {
        return -1;
    }
    
    /* Allocate a new directory entry */
    struct ramfs_dirent *new_entries = krealloc(ramfs_dir->entries, (ramfs_dir->count + 1) * sizeof(struct ramfs_dirent), MEM_KERNEL);
    
    if (new_entries == NULL) {
        return -1;
    }
    
    /* Set the new entries */
    ramfs_dir->entries = new_entries;
    
    /* Get the new directory entry */
    dirent = &ramfs_dir->entries[ramfs_dir->count];
    
    /* Initialize the directory entry */
    dirent->name = kmalloc(len + 1, MEM_KERNEL);
    
    if (dirent->name == NULL) {
        return -1;
    }
    
    /* Copy the name */
    memcpy(dirent->name, name, len);
    dirent->name[len] = '\0';
    
    /* Set the directory entry */
    dirent->len = len;
    dirent->ino = inode->i_ino;
    
    /* Set the entry type */
    if (S_ISDIR(inode->i_mode)) {
        dirent->type = DT_DIR;
    } else if (S_ISREG(inode->i_mode)) {
        dirent->type = DT_REG;
    } else if (S_ISLNK(inode->i_mode)) {
        dirent->type = DT_LNK;
    } else if (S_ISBLK(inode->i_mode)) {
        dirent->type = DT_BLK;
    } else if (S_ISCHR(inode->i_mode)) {
        dirent->type = DT_CHR;
    } else if (S_ISFIFO(inode->i_mode)) {
        dirent->type = DT_FIFO;
    } else if (S_ISSOCK(inode->i_mode)) {
        dirent->type = DT_SOCK;
    } else {
        dirent->type = DT_UNKNOWN;
    }
    
    /* Set the entry mode */
    dirent->mode = inode->i_mode;
    
    /* Increment the entry count */
    ramfs_dir->count++;
    
    return 0;
}

/* Remove a directory entry */
int ramfs_remove_dirent(struct inode *dir, const char *name, int len) {
    if (dir == NULL || name == NULL) {
        return -1;
    }
    
    /* Get the RAM file system inode */
    struct ramfs_inode *ramfs_inode = container_of(dir, struct ramfs_inode, vfs_inode);
    
    /* Get the directory */
    struct ramfs_dir *ramfs_dir = (struct ramfs_dir *)ramfs_inode->data;
    
    if (ramfs_dir == NULL) {
        return -1;
    }
    
    /* Find the directory entry */
    int index = -1;
    
    for (int i = 0; i < ramfs_dir->count; i++) {
        /* Get the directory entry */
        struct ramfs_dirent *dirent = &ramfs_dir->entries[i];
        
        /* Check if the name matches */
        if (dirent->len == len && memcmp(dirent->name, name, len) == 0) {
            index = i;
            break;
        }
    }
    
    /* Check if the entry was found */
    if (index < 0) {
        return -1;
    }
    
    /* Free the entry name */
    kfree(ramfs_dir->entries[index].name);
    
    /* Remove the entry */
    for (int i = index; i < ramfs_dir->count - 1; i++) {
        ramfs_dir->entries[i] = ramfs_dir->entries[i + 1];
    }
    
    /* Decrement the entry count */
    ramfs_dir->count--;
    
    /* Check if the directory is empty */
    if (ramfs_dir->count == 0) {
        /* Free the entries */
        kfree(ramfs_dir->entries);
        ramfs_dir->entries = NULL;
    } else {
        /* Resize the entries */
        struct ramfs_dirent *new_entries = krealloc(ramfs_dir->entries, ramfs_dir->count * sizeof(struct ramfs_dirent), MEM_KERNEL);
        
        if (new_entries != NULL) {
            ramfs_dir->entries = new_entries;
        }
    }
    
    return 0;
}

/* Check if a directory is empty */
int ramfs_empty_dir(struct inode *dir) {
    if (dir == NULL) {
        return 0;
    }
    
    /* Get the RAM file system inode */
    struct ramfs_inode *ramfs_inode = container_of(dir, struct ramfs_inode, vfs_inode);
    
    /* Get the directory */
    struct ramfs_dir *ramfs_dir = (struct ramfs_dir *)ramfs_inode->data;
    
    if (ramfs_dir == NULL) {
        return 1;
    }
    
    /* Check if the directory is empty */
    return ramfs_dir->count == 0;
}

/* Set a symbolic link */
int ramfs_set_link(struct inode *inode, const char *symname) {
    if (inode == NULL || symname == NULL) {
        return -1;
    }
    
    /* Get the RAM file system inode */
    struct ramfs_inode *ramfs_inode = container_of(inode, struct ramfs_inode, vfs_inode);
    
    /* Get the symlink length */
    int len = strlen(symname);
    
    /* Allocate memory for the symlink */
    char *link = kmalloc(len + 1, MEM_KERNEL);
    
    if (link == NULL) {
        return -1;
    }
    
    /* Copy the symlink */
    memcpy(link, symname, len);
    link[len] = '\0';
    
    /* Set the symlink */
    ramfs_inode->data = link;
    ramfs_inode->size = len;
    
    /* Update the inode size */
    inode->i_size = len;
    
    return 0;
}

/* Get a symbolic link */
const char *ramfs_get_link(struct inode *inode) {
    if (inode == NULL) {
        return NULL;
    }
    
    /* Get the RAM file system inode */
    struct ramfs_inode *ramfs_inode = container_of(inode, struct ramfs_inode, vfs_inode);
    
    /* Return the symlink */
    return (const char *)ramfs_inode->data;
}
