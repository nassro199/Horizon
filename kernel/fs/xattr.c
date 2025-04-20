/**
 * xattr.c - Horizon kernel extended attribute operations
 * 
 * This file contains the implementation of extended attribute operations.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/fs/vfs.h>
#include <horizon/fs/file.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Extended attribute namespaces */
#define XATTR_USER_PREFIX     "user."
#define XATTR_SYSTEM_PREFIX   "system."
#define XATTR_TRUSTED_PREFIX  "trusted."
#define XATTR_SECURITY_PREFIX "security."

/* Extended attribute flags */
#define XATTR_CREATE  0x1     /* Create the attribute if it does not exist */
#define XATTR_REPLACE 0x2     /* Replace the attribute if it exists */

/**
 * Get an extended attribute
 * 
 * @param path The path to the file
 * @param name The name of the attribute
 * @param value The buffer to store the attribute value
 * @param size The size of the buffer
 * @return The size of the attribute value, or a negative error code
 */
ssize_t file_getxattr(const char *path, const char *name, void *value, size_t size) {
    /* Check parameters */
    if (path == NULL || name == NULL) {
        return -1;
    }
    
    /* Get the path */
    struct path p;
    int error = vfs_kern_path(path, 0, &p);
    
    if (error) {
        return error;
    }
    
    /* Get the extended attribute */
    ssize_t result = vfs_getxattr(p.dentry, name, value, size);
    
    /* Release the path */
    vfs_path_release(&p);
    
    return result;
}

/**
 * Set an extended attribute
 * 
 * @param path The path to the file
 * @param name The name of the attribute
 * @param value The attribute value
 * @param size The size of the attribute value
 * @param flags The flags
 * @return 0 on success, or a negative error code
 */
int file_setxattr(const char *path, const char *name, const void *value, size_t size, int flags) {
    /* Check parameters */
    if (path == NULL || name == NULL || value == NULL) {
        return -1;
    }
    
    /* Get the path */
    struct path p;
    int error = vfs_kern_path(path, 0, &p);
    
    if (error) {
        return error;
    }
    
    /* Set the extended attribute */
    error = vfs_setxattr(p.dentry, name, value, size, flags);
    
    /* Release the path */
    vfs_path_release(&p);
    
    return error;
}

/**
 * List extended attributes
 * 
 * @param path The path to the file
 * @param list The buffer to store the attribute names
 * @param size The size of the buffer
 * @return The size of the attribute names, or a negative error code
 */
ssize_t file_listxattr(const char *path, char *list, size_t size) {
    /* Check parameters */
    if (path == NULL) {
        return -1;
    }
    
    /* Get the path */
    struct path p;
    int error = vfs_kern_path(path, 0, &p);
    
    if (error) {
        return error;
    }
    
    /* List the extended attributes */
    ssize_t result = vfs_listxattr(p.dentry, list, size);
    
    /* Release the path */
    vfs_path_release(&p);
    
    return result;
}

/**
 * Remove an extended attribute
 * 
 * @param path The path to the file
 * @param name The name of the attribute
 * @return 0 on success, or a negative error code
 */
int file_removexattr(const char *path, const char *name) {
    /* Check parameters */
    if (path == NULL || name == NULL) {
        return -1;
    }
    
    /* Get the path */
    struct path p;
    int error = vfs_kern_path(path, 0, &p);
    
    if (error) {
        return error;
    }
    
    /* Remove the extended attribute */
    error = vfs_removexattr(p.dentry, name);
    
    /* Release the path */
    vfs_path_release(&p);
    
    return error;
}

/**
 * Get an extended attribute (don't follow symbolic links)
 * 
 * @param path The path to the file
 * @param name The name of the attribute
 * @param value The buffer to store the attribute value
 * @param size The size of the buffer
 * @return The size of the attribute value, or a negative error code
 */
ssize_t file_lgetxattr(const char *path, const char *name, void *value, size_t size) {
    /* Check parameters */
    if (path == NULL || name == NULL) {
        return -1;
    }
    
    /* Get the path */
    struct path p;
    int error = vfs_kern_path(path, LOOKUP_NOFOLLOW, &p);
    
    if (error) {
        return error;
    }
    
    /* Get the extended attribute */
    ssize_t result = vfs_getxattr(p.dentry, name, value, size);
    
    /* Release the path */
    vfs_path_release(&p);
    
    return result;
}

/**
 * Set an extended attribute (don't follow symbolic links)
 * 
 * @param path The path to the file
 * @param name The name of the attribute
 * @param value The attribute value
 * @param size The size of the attribute value
 * @param flags The flags
 * @return 0 on success, or a negative error code
 */
int file_lsetxattr(const char *path, const char *name, const void *value, size_t size, int flags) {
    /* Check parameters */
    if (path == NULL || name == NULL || value == NULL) {
        return -1;
    }
    
    /* Get the path */
    struct path p;
    int error = vfs_kern_path(path, LOOKUP_NOFOLLOW, &p);
    
    if (error) {
        return error;
    }
    
    /* Set the extended attribute */
    error = vfs_setxattr(p.dentry, name, value, size, flags);
    
    /* Release the path */
    vfs_path_release(&p);
    
    return error;
}

/**
 * List extended attributes (don't follow symbolic links)
 * 
 * @param path The path to the file
 * @param list The buffer to store the attribute names
 * @param size The size of the buffer
 * @return The size of the attribute names, or a negative error code
 */
ssize_t file_llistxattr(const char *path, char *list, size_t size) {
    /* Check parameters */
    if (path == NULL) {
        return -1;
    }
    
    /* Get the path */
    struct path p;
    int error = vfs_kern_path(path, LOOKUP_NOFOLLOW, &p);
    
    if (error) {
        return error;
    }
    
    /* List the extended attributes */
    ssize_t result = vfs_listxattr(p.dentry, list, size);
    
    /* Release the path */
    vfs_path_release(&p);
    
    return result;
}

/**
 * Remove an extended attribute (don't follow symbolic links)
 * 
 * @param path The path to the file
 * @param name The name of the attribute
 * @return 0 on success, or a negative error code
 */
int file_lremovexattr(const char *path, const char *name) {
    /* Check parameters */
    if (path == NULL || name == NULL) {
        return -1;
    }
    
    /* Get the path */
    struct path p;
    int error = vfs_kern_path(path, LOOKUP_NOFOLLOW, &p);
    
    if (error) {
        return error;
    }
    
    /* Remove the extended attribute */
    error = vfs_removexattr(p.dentry, name);
    
    /* Release the path */
    vfs_path_release(&p);
    
    return error;
}

/**
 * Get an extended attribute (file descriptor)
 * 
 * @param file The file
 * @param name The name of the attribute
 * @param value The buffer to store the attribute value
 * @param size The size of the buffer
 * @return The size of the attribute value, or a negative error code
 */
ssize_t file_fgetxattr(file_t *file, const char *name, void *value, size_t size) {
    /* Check parameters */
    if (file == NULL || name == NULL) {
        return -1;
    }
    
    /* Get the extended attribute */
    return vfs_getxattr(file->f_path.dentry, name, value, size);
}

/**
 * Set an extended attribute (file descriptor)
 * 
 * @param file The file
 * @param name The name of the attribute
 * @param value The attribute value
 * @param size The size of the attribute value
 * @param flags The flags
 * @return 0 on success, or a negative error code
 */
int file_fsetxattr(file_t *file, const char *name, const void *value, size_t size, int flags) {
    /* Check parameters */
    if (file == NULL || name == NULL || value == NULL) {
        return -1;
    }
    
    /* Set the extended attribute */
    return vfs_setxattr(file->f_path.dentry, name, value, size, flags);
}

/**
 * List extended attributes (file descriptor)
 * 
 * @param file The file
 * @param list The buffer to store the attribute names
 * @param size The size of the buffer
 * @return The size of the attribute names, or a negative error code
 */
ssize_t file_flistxattr(file_t *file, char *list, size_t size) {
    /* Check parameters */
    if (file == NULL) {
        return -1;
    }
    
    /* List the extended attributes */
    return vfs_listxattr(file->f_path.dentry, list, size);
}

/**
 * Remove an extended attribute (file descriptor)
 * 
 * @param file The file
 * @param name The name of the attribute
 * @return 0 on success, or a negative error code
 */
int file_fremovexattr(file_t *file, const char *name) {
    /* Check parameters */
    if (file == NULL || name == NULL) {
        return -1;
    }
    
    /* Remove the extended attribute */
    return vfs_removexattr(file->f_path.dentry, name);
}
