/**
 * fs.c - Horizon kernel file system implementation
 *
 * This file contains the implementation of the virtual file system.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/fs.h>
#include <horizon/fs/ext2.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <horizon/spinlock.h>
#include <horizon/printk.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Maximum number of file systems */
#define MAX_FS_TYPES 16

/* Maximum number of mount points */
#define MAX_MOUNTS 32

/* File system type structure */
typedef struct fs_type {
    char name[32];                                   /* File system name */
    int (*mount)(const char *dev, const char *dir, u32 flags); /* Mount function */
    int (*unmount)(const char *dir);                 /* Unmount function */
} fs_type_t;

/* Mount point structure */
typedef struct mount_point {
    char dev[64];                                    /* Device name */
    char dir[256];                                   /* Mount directory */
    fs_type_t *fs;                                   /* File system type */
    u32 flags;                                       /* Mount flags */
    struct super_block *super;                       /* Superblock */
    struct inode *root;                              /* Root inode */
} mount_point_t;

/* File system types */
static fs_type_t fs_types[MAX_FS_TYPES];
static int fs_type_count = 0;

/* Mount points */
static mount_point_t mounts[MAX_MOUNTS];
static int mount_count = 0;

/* File system lock */
static spinlock_t fs_lock = SPIN_LOCK_INITIALIZER;

/**
 * Initialize the file system
 */
void fs_init(void) {
    /* Initialize the file system types */
    memset(fs_types, 0, sizeof(fs_types));
    fs_type_count = 0;

    /* Initialize the mount points */
    memset(mounts, 0, sizeof(mounts));
    mount_count = 0;

    /* Initialize the ext2 file system */
    ext2_init();

    printk(KERN_INFO "FS: Initialized file system\n");
}

/**
 * Find a file system type by name
 *
 * @param name File system name
 * @return Pointer to the file system type, or NULL if not found
 */
static fs_type_t *fs_find_by_name(const char *name) {
    /* Check parameters */
    if (name == NULL) {
        return NULL;
    }

    /* Search for the file system type */
    for (int i = 0; i < fs_type_count; i++) {
        if (strcmp(fs_types[i].name, name) == 0) {
            return &fs_types[i];
        }
    }

    /* Not found */
    return NULL;
}

/**
 * Register a file system
 *
 * @param name File system name
 * @param mount Mount function
 * @param unmount Unmount function
 * @return 0 on success, negative error code on failure
 */
int fs_register(const char *name, int (*mount)(const char *dev, const char *dir, u32 flags), int (*unmount)(const char *dir)) {
    /* Check parameters */
    if (name == NULL || mount == NULL || unmount == NULL) {
        return -EINVAL;
    }

    /* Lock the file system */
    spin_lock(&fs_lock);

    /* Check if the file system is already registered */
    for (int i = 0; i < fs_type_count; i++) {
        if (strcmp(fs_types[i].name, name) == 0) {
            /* File system already registered */
            spin_unlock(&fs_lock);
            return -EEXIST;
        }
    }

    /* Check if there is space for a new file system */
    if (fs_type_count >= MAX_FS_TYPES) {
        /* No space for a new file system */
        spin_unlock(&fs_lock);
        return -ENOMEM;
    }

    /* Register the file system */
    strncpy(fs_types[fs_type_count].name, name, 31);
    fs_types[fs_type_count].name[31] = '\0';
    fs_types[fs_type_count].mount = mount;
    fs_types[fs_type_count].unmount = unmount;
    fs_type_count++;

    /* Unlock the file system */
    spin_unlock(&fs_lock);

    printk(KERN_INFO "FS: Registered file system '%s'\n", name);

    return 0;
}

/**
 * Find a file system by name
 *
 * @param name File system name
 * @return Pointer to the file system, or NULL if not found
 */
static fs_type_t *fs_find(const char *name) {
    /* Check parameters */
    if (name == NULL) {
        return NULL;
    }

    /* Find the file system */
    for (int i = 0; i < fs_type_count; i++) {
        if (strcmp(fs_types[i].name, name) == 0) {
            /* Found the file system */
            return &fs_types[i];
        }
    }

    /* File system not found */
    return NULL;
}

/**
 * Find a mount point by directory
 *
 * @param dir Directory
 * @return Pointer to the mount point, or NULL if not found
 */
static mount_point_t *mount_find_by_dir(const char *dir) {
    /* Check parameters */
    if (dir == NULL) {
        return NULL;
    }

    /* Find the mount point */
    for (int i = 0; i < mount_count; i++) {
        if (strcmp(mounts[i].dir, dir) == 0) {
            /* Found the mount point */
            return &mounts[i];
        }
    }

    /* Mount point not found */
    return NULL;
}

/**
 * Mount a file system
 *
 * @param dev Device name
 * @param dir Directory to mount on
 * @param fs_name File system name
 * @param flags Mount flags
 * @return 0 on success, negative error code on failure
 */
int fs_mount(const char *dev, const char *dir, const char *fs_name, u32 flags) {
    /* Check parameters */
    if (dev == NULL || dir == NULL || fs_name == NULL) {
        return -EINVAL;
    }

    /* Lock the file system */
    spin_lock(&fs_lock);

    /* Find the file system */
    fs_type_t *fs = fs_find_by_name(fs_name);
    if (fs == NULL) {
        /* File system not found */
        spin_unlock(&fs_lock);
        return -ENODEV;
    }

    /* Check if the directory is already mounted */
    if (mount_find_by_dir(dir) != NULL) {
        /* Directory already mounted */
        spin_unlock(&fs_lock);
        return -EBUSY;
    }

    /* Check if there is space for a new mount point */
    if (mount_count >= MAX_MOUNTS) {
        /* No space for a new mount point */
        spin_unlock(&fs_lock);
        return -ENOMEM;
    }

    /* Mount the file system */
    int ret = fs->mount(dev, dir, flags);
    if (ret < 0) {
        /* Failed to mount the file system */
        spin_unlock(&fs_lock);
        return ret;
    }

    /* Add the mount point */
    strncpy(mounts[mount_count].dev, dev, 63);
    mounts[mount_count].dev[63] = '\0';
    strncpy(mounts[mount_count].dir, dir, 255);
    mounts[mount_count].dir[255] = '\0';
    mounts[mount_count].fs = fs;
    mounts[mount_count].flags = flags;
    mount_count++;

    /* Unlock the file system */
    spin_unlock(&fs_lock);

    printk(KERN_INFO "FS: Mounted '%s' on '%s' with file system '%s'\n", dev, dir, fs_name);

    return 0;
}

/**
 * Mount a superblock on a directory
 *
 * @param dir Directory to mount on
 * @param super Superblock to mount
 * @return 0 on success, negative error code on failure
 */
int fs_mount_super(const char *dir, struct super_block *super) {
    /* Check parameters */
    if (dir == NULL || super == NULL) {
        return -EINVAL;
    }

    /* Lock the file system */
    spin_lock(&fs_lock);

    /* Check if the directory is already mounted */
    if (mount_find_by_dir(dir) != NULL) {
        /* Directory is already mounted */
        spin_unlock(&fs_lock);
        return -EBUSY;
    }

    /* Check if there is space for a new mount point */
    if (mount_count >= MAX_MOUNTS) {
        /* No space for a new mount point */
        spin_unlock(&fs_lock);
        return -ENOMEM;
    }

    /* Create a root inode */
    inode_t *root = kmalloc(sizeof(inode_t), 0);

    if (root == NULL) {
        spin_unlock(&fs_lock);
        return -ENOMEM;
    }

    /* Initialize the root inode */
    memset(root, 0, sizeof(inode_t));
    root->inode_num = 2; /* Root inode number */
    root->type = FILE_TYPE_DIRECTORY;
    root->permissions = 0755;
    root->uid = 0;
    root->gid = 0;
    root->size = 4096;
    root->blocks = 8;
    root->atime = 0;
    root->mtime = 0;
    root->ctime = 0;
    root->links = 1;

    /* Add the mount point */
    strncpy(mounts[mount_count].dir, dir, 255);
    mounts[mount_count].dir[255] = '\0';
    mounts[mount_count].super = super;
    mounts[mount_count].root = root;
    mount_count++;

    /* Unlock the file system */
    spin_unlock(&fs_lock);

    printk(KERN_INFO "FS: Mounted superblock on '%s'\n", dir);

    return 0;
}

/**
 * Unmount a superblock
 *
 * @param dir Directory to unmount
 * @return 0 on success, negative error code on failure
 */
int fs_unmount_super(const char *dir) {
    /* Check parameters */
    if (dir == NULL) {
        return -EINVAL;
    }

    /* Lock the file system */
    spin_lock(&fs_lock);

    /* Find the mount point */
    mount_point_t *mount = mount_find_by_dir(dir);

    if (mount == NULL) {
        /* Mount point not found */
        spin_unlock(&fs_lock);
        return -EINVAL;
    }

    /* Free the root inode */
    if (mount->root != NULL) {
        kfree(mount->root);
    }

    /* Remove the mount point */
    int i;
    for (i = 0; i < mount_count; i++) {
        if (&mounts[i] == mount) {
            /* Found the mount point */
            break;
        }
    }

    /* Shift the mount points */
    for (int j = i; j < mount_count - 1; j++) {
        mounts[j] = mounts[j + 1];
    }

    /* Decrement the mount count */
    mount_count--;

    /* Unlock the file system */
    spin_unlock(&fs_lock);

    printk(KERN_INFO "FS: Unmounted superblock from '%s'\n", dir);

    return 0;
}

/**
 * Get the superblock for a directory
 *
 * @param dir Directory to get superblock for
 * @return Pointer to the superblock, or NULL on failure
 */
struct super_block *fs_get_super(const char *dir) {
    /* Check parameters */
    if (dir == NULL) {
        return NULL;
    }

    /* Lock the file system */
    spin_lock(&fs_lock);

    /* Find the mount point */
    mount_point_t *mount = mount_find_by_dir(dir);

    if (mount == NULL) {
        /* Mount point not found */
        spin_unlock(&fs_lock);
        return NULL;
    }

    /* Get the superblock */
    struct super_block *super = mount->super;

    /* Unlock the file system */
    spin_unlock(&fs_lock);

    return super;
}

/**
 * Lookup a file
 *
 * @param path Path to look up
 * @return Pointer to the inode, or NULL on failure
 */
struct inode *fs_lookup(const char *path) {
    /* Check parameters */
    if (path == NULL) {
        return NULL;
    }

    /* Find the mount point */
    /* This would be implemented with actual mount point finding */

    /* Look up the file */
    /* This would be implemented with actual file lookup */

    return NULL;
}

/**
 * Unmount a file system
 *
 * @param dir Directory to unmount
 * @return 0 on success, negative error code on failure
 */
int fs_unmount(const char *dir) {
    /* Check parameters */
    if (dir == NULL) {
        return -EINVAL;
    }

    /* Lock the file system */
    spin_lock(&fs_lock);

    /* Find the mount point */
    mount_point_t *mount = mount_find_by_dir(dir);
    if (mount == NULL) {
        /* Mount point not found */
        spin_unlock(&fs_lock);
        return -EINVAL;
    }

    /* Unmount the file system */
    int ret = mount->fs->unmount(dir);
    if (ret < 0) {
        /* Failed to unmount the file system */
        spin_unlock(&fs_lock);
        return ret;
    }

    /* Remove the mount point */
    int i;
    for (i = 0; i < mount_count; i++) {
        if (&mounts[i] == mount) {
            /* Found the mount point */
            break;
        }
    }

    /* Shift the mount points */
    for (int j = i; j < mount_count - 1; j++) {
        mounts[j] = mounts[j + 1];
    }

    /* Decrement the mount count */
    mount_count--;

    /* Unlock the file system */
    spin_unlock(&fs_lock);

    printk(KERN_INFO "FS: Unmounted '%s'\n", dir);

    return 0;
}

/**
 * Open a file
 *
 * @param path File path
 * @param flags Open flags
 * @return Pointer to the file, or NULL on failure
 */
file_t *fs_open(const char *path, u32 flags) {
    /* Check parameters */
    if (path == NULL) {
        return NULL;
    }

    /* Allocate a file structure */
    file_t *file = kmalloc(sizeof(file_t), MEM_KERNEL | MEM_ZERO);
    if (file == NULL) {
        return NULL;
    }

    /* Set the file name */
    strncpy(file->name, path, 255);
    file->name[255] = '\0';

    /* Set the file position */
    file->position = 0;

    /* Find the mount point */
    /* This would be implemented with actual mount point finding */

    /* Open the file */
    if (file->f_ops != NULL && file->f_ops->open != NULL) {
        error_t ret = file->f_ops->open(file, flags);
        if (ret != 0) {
            /* Failed to open the file */
            kfree(file);
            return NULL;
        }
    }

    return file;
}

/**
 * Close a file
 *
 * @param file File to close
 * @return 0 on success, negative error code on failure
 */
error_t fs_close(file_t *file) {
    /* Check parameters */
    if (file == NULL) {
        return -EINVAL;
    }

    /* Close the file */
    if (file->f_ops != NULL && file->f_ops->close != NULL) {
        error_t ret = file->f_ops->close(file);
        if (ret != 0) {
            /* Failed to close the file */
            return ret;
        }
    }

    /* Free the file structure */
    kfree(file);

    return 0;
}

/**
 * Read from a file
 *
 * @param file File to read from
 * @param buffer Buffer to read into
 * @param size Number of bytes to read
 * @return Number of bytes read, or negative error code on failure
 */
ssize_t fs_read(file_t *file, void *buffer, size_t size) {
    /* Check parameters */
    if (file == NULL || buffer == NULL) {
        return -EINVAL;
    }

    /* Read from the file */
    if (file->f_ops != NULL && file->f_ops->read != NULL) {
        return file->f_ops->read(file, buffer, size);
    }

    return -ENOSYS;
}

/**
 * Write to a file
 *
 * @param file File to write to
 * @param buffer Buffer to write from
 * @param size Number of bytes to write
 * @return Number of bytes written, or negative error code on failure
 */
ssize_t fs_write(file_t *file, const void *buffer, size_t size) {
    /* Check parameters */
    if (file == NULL || buffer == NULL) {
        return -EINVAL;
    }

    /* Write to the file */
    if (file->f_ops != NULL && file->f_ops->write != NULL) {
        return file->f_ops->write(file, buffer, size);
    }

    return -ENOSYS;
}

/**
 * Seek in a file
 *
 * @param file File to seek in
 * @param offset Offset to seek to
 * @param whence Seek origin
 * @return 0 on success, negative error code on failure
 */
error_t fs_seek(file_t *file, u64 offset, int whence) {
    /* Check parameters */
    if (file == NULL) {
        return -EINVAL;
    }

    /* Seek in the file */
    if (file->f_ops != NULL && file->f_ops->seek != NULL) {
        return file->f_ops->seek(file, offset, whence);
    }

    return -ENOSYS;
}
