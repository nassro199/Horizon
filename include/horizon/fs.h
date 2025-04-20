/**
 * fs.h - File system definitions
 *
 * This file contains definitions for the virtual file system.
 */

#ifndef _FS_H
#define _FS_H

#include <horizon/types.h>

/* File types */
typedef enum {
    FILE_TYPE_REGULAR,
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_CHAR_DEVICE,
    FILE_TYPE_BLOCK_DEVICE,
    FILE_TYPE_PIPE,
    FILE_TYPE_SYMLINK
} file_type_t;

/* File permissions */
#define FILE_PERM_READ    0x04
#define FILE_PERM_WRITE   0x02
#define FILE_PERM_EXEC    0x01

/* File open flags */
#define FILE_OPEN_READ    0x01
#define FILE_OPEN_WRITE   0x02
#define FILE_OPEN_CREATE  0x04
#define FILE_OPEN_APPEND  0x08
#define FILE_OPEN_TRUNC   0x10

/* File structure */
typedef struct file {
    char name[256];           /* File name */
    file_type_t type;         /* File type */
    u32 permissions;          /* File permissions */
    u64 size;                 /* File size */
    u64 position;             /* Current position */
    void *fs_data;            /* File system specific data */
    struct file_operations *ops; /* File operations */
} file_t;

/* File operations */
typedef struct file_operations {
    ssize_t (*read)(file_t *file, void *buffer, size_t size);
    ssize_t (*write)(file_t *file, const void *buffer, size_t size);
    error_t (*open)(file_t *file, u32 flags);
    error_t (*close)(file_t *file);
    error_t (*seek)(file_t *file, u64 offset, int whence);
} file_operations_t;

/* Mount flags */
#define MOUNT_READ_ONLY   0x01    /* Read-only mount */
#define MOUNT_NO_EXEC     0x02    /* Do not allow execution of binaries */
#define MOUNT_NO_DEV      0x04    /* Do not interpret character or block special devices */
#define MOUNT_NO_SUID     0x08    /* Do not allow set-user-identifier or set-group-identifier bits to take effect */

/* File system functions */
void fs_init(void);
int fs_register(const char *name, int (*mount)(const char *dev, const char *dir, u32 flags), int (*unmount)(const char *dir));
int fs_mount(const char *dev, const char *dir, const char *fs_name, u32 flags);
int fs_unmount(const char *dir);
file_t *fs_open(const char *path, u32 flags);
error_t fs_close(file_t *file);
ssize_t fs_read(file_t *file, void *buffer, size_t size);
ssize_t fs_write(file_t *file, const void *buffer, size_t size);
error_t fs_seek(file_t *file, u64 offset, int whence);

#endif /* _FS_H */
