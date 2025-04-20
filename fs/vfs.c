/**
 * vfs.c - Virtual File System implementation
 *
 * This file contains the implementation of the virtual file system.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/fs.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Maximum number of open files */
#define MAX_OPEN_FILES 64

/* Maximum number of mount points */
#define MAX_MOUNT_POINTS 16

/* Open file table */
static file_t *open_files[MAX_OPEN_FILES];

/* Mount point structure */
typedef struct mount_point {
    char dev[64];              /* Device name */
    char dir[256];             /* Mount directory */
    u32 flags;                 /* Mount flags */
    struct file_system *fs;    /* File system */
} mount_point_t;

/* File system structure */
typedef struct file_system {
    char name[16];             /* File system name */
    int (*mount)(const char *dev, const char *dir, u32 flags);
    int (*unmount)(const char *dir);
} file_system_t;

/* Mount point table */
static mount_point_t mount_points[MAX_MOUNT_POINTS];

/* File system table */
static file_system_t *file_systems = NULL;

/* Register a file system */
int fs_register(const char *name, int (*mount)(const char *dev, const char *dir, u32 flags), int (*unmount)(const char *dir)) {
    /* Allocate a file system structure */
    file_system_t *fs = kmalloc(sizeof(file_system_t), MEM_KERNEL | MEM_ZERO);

    if (fs == NULL) {
        return -1;
    }

    /* Copy the file system name */
    strncpy(fs->name, name, 15);
    fs->name[15] = '\0';

    /* Set the mount and unmount functions */
    fs->mount = mount;
    fs->unmount = unmount;

    /* Add to the file system table */
    fs->next = file_systems;
    file_systems = fs;

    return 0;
}

/* Mount a file system */
int fs_mount(const char *dev, const char *dir, const char *fs_name, u32 flags) {
    /* Find the file system */
    file_system_t *fs = file_systems;
    while (fs != NULL) {
        if (strcmp(fs->name, fs_name) == 0) {
            break;
        }
        fs = fs->next;
    }

    if (fs == NULL) {
        return -1;
    }

    /* Find a free mount point */
    int i;
    for (i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (mount_points[i].fs == NULL) {
            break;
        }
    }

    if (i == MAX_MOUNT_POINTS) {
        return -1;
    }

    /* Mount the file system */
    int result = fs->mount(dev, dir, flags);

    if (result < 0) {
        return result;
    }

    /* Set up the mount point */
    strncpy(mount_points[i].dev, dev, 63);
    mount_points[i].dev[63] = '\0';

    strncpy(mount_points[i].dir, dir, 255);
    mount_points[i].dir[255] = '\0';

    mount_points[i].flags = flags;
    mount_points[i].fs = fs;

    return 0;
}

/* Unmount a file system */
int fs_unmount(const char *dir) {
    /* Find the mount point */
    int i;
    for (i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (mount_points[i].fs != NULL && strcmp(mount_points[i].dir, dir) == 0) {
            break;
        }
    }

    if (i == MAX_MOUNT_POINTS) {
        return -1;
    }

    /* Unmount the file system */
    int result = mount_points[i].fs->unmount(dir);

    if (result < 0) {
        return result;
    }

    /* Clear the mount point */
    mount_points[i].fs = NULL;

    return 0;
}

/* Initialize the file system */
void fs_init(void) {
    /* Initialize the open file table */
    for (u32 i = 0; i < MAX_OPEN_FILES; i++) {
        open_files[i] = NULL;
    }

    /* Initialize the mount point table */
    for (u32 i = 0; i < MAX_MOUNT_POINTS; i++) {
        mount_points[i].fs = NULL;
    }

    /* Initialize the file system table */
    file_systems = NULL;

    /* Register file systems */
    /* This would register actual file systems */

    /* Mount the root file system */
    /* This would mount an actual file system */
}

/* Find a free file descriptor */
static int find_free_fd(void) {
    for (u32 i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i] == NULL) {
            return i;
        }
    }

    return -1;
}

/* Open a file */
file_t *fs_open(const char *path, u32 flags) {
    if (path == NULL) {
        return NULL;
    }

    /* Find a free file descriptor */
    int fd = find_free_fd();

    if (fd < 0) {
        return NULL;
    }

    /* Allocate a file structure */
    file_t *file = kmalloc(sizeof(file_t), MEM_KERNEL | MEM_ZERO);

    if (file == NULL) {
        return NULL;
    }

    /* Copy the file name */
    u32 i;
    for (i = 0; i < 255 && path[i] != '\0'; i++) {
        file->name[i] = path[i];
    }
    file->name[i] = '\0';

    /* Set default values */
    file->type = FILE_TYPE_REGULAR;
    file->permissions = FILE_PERM_READ | FILE_PERM_WRITE;
    file->size = 0;
    file->position = 0;

    /* This would be implemented with a real file system */
    /* For now, just return the file structure */

    /* Add to the open file table */
    open_files[fd] = file;

    return file;
}

/* Close a file */
error_t fs_close(file_t *file) {
    if (file == NULL) {
        return ERROR_INVAL;
    }

    /* Find the file in the open file table */
    for (u32 i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i] == file) {
            /* Remove from the open file table */
            open_files[i] = NULL;

            /* Free the file structure */
            kfree(file);

            return SUCCESS;
        }
    }

    return ERROR_INVAL;
}

/* Read from a file */
ssize_t fs_read(file_t *file, void *buffer, size_t size) {
    if (file == NULL || buffer == NULL) {
        return ERROR_INVAL;
    }

    /* Check if the file has read operations */
    if (file->ops == NULL || file->ops->read == NULL) {
        return ERROR_INVAL;
    }

    /* Call the file's read operation */
    return file->ops->read(file, buffer, size);
}

/* Write to a file */
ssize_t fs_write(file_t *file, const void *buffer, size_t size) {
    if (file == NULL || buffer == NULL) {
        return ERROR_INVAL;
    }

    /* Check if the file has write operations */
    if (file->ops == NULL || file->ops->write == NULL) {
        return ERROR_INVAL;
    }

    /* Call the file's write operation */
    return file->ops->write(file, buffer, size);
}

/* Seek in a file */
error_t fs_seek(file_t *file, u64 offset, int whence) {
    if (file == NULL) {
        return ERROR_INVAL;
    }

    /* Check if the file has seek operations */
    if (file->ops == NULL || file->ops->seek == NULL) {
        return ERROR_INVAL;
    }

    /* Call the file's seek operation */
    return file->ops->seek(file, offset, whence);
}
