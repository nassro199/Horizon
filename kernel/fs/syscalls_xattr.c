/**
 * syscalls_xattr.c - Horizon kernel extended attribute system calls
 * 
 * This file contains the implementation of the extended attribute system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/fs/vfs.h>
#include <horizon/fs/file.h>
#include <horizon/mm.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: getxattr */
long sys_getxattr(long pathname, long name, long value, long size, long unused1, long unused2) {
    /* Get the extended attribute */
    return file_getxattr((const char *)pathname, (const char *)name, (void *)value, size);
}

/* System call: lgetxattr */
long sys_lgetxattr(long pathname, long name, long value, long size, long unused1, long unused2) {
    /* Get the extended attribute (don't follow symbolic links) */
    return file_lgetxattr((const char *)pathname, (const char *)name, (void *)value, size);
}

/* System call: fgetxattr */
long sys_fgetxattr(long fd, long name, long value, long size, long unused1, long unused2) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Get the extended attribute */
    return file_fgetxattr(file, (const char *)name, (void *)value, size);
}

/* System call: setxattr */
long sys_setxattr(long pathname, long name, long value, long size, long flags, long unused1) {
    /* Set the extended attribute */
    return file_setxattr((const char *)pathname, (const char *)name, (const void *)value, size, flags);
}

/* System call: lsetxattr */
long sys_lsetxattr(long pathname, long name, long value, long size, long flags, long unused1) {
    /* Set the extended attribute (don't follow symbolic links) */
    return file_lsetxattr((const char *)pathname, (const char *)name, (const void *)value, size, flags);
}

/* System call: fsetxattr */
long sys_fsetxattr(long fd, long name, long value, long size, long flags, long unused1) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Set the extended attribute */
    return file_fsetxattr(file, (const char *)name, (const void *)value, size, flags);
}

/* System call: listxattr */
long sys_listxattr(long pathname, long list, long size, long unused1, long unused2, long unused3) {
    /* List the extended attributes */
    return file_listxattr((const char *)pathname, (char *)list, size);
}

/* System call: llistxattr */
long sys_llistxattr(long pathname, long list, long size, long unused1, long unused2, long unused3) {
    /* List the extended attributes (don't follow symbolic links) */
    return file_llistxattr((const char *)pathname, (char *)list, size);
}

/* System call: flistxattr */
long sys_flistxattr(long fd, long list, long size, long unused1, long unused2, long unused3) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* List the extended attributes */
    return file_flistxattr(file, (char *)list, size);
}

/* System call: removexattr */
long sys_removexattr(long pathname, long name, long unused1, long unused2, long unused3, long unused4) {
    /* Remove the extended attribute */
    return file_removexattr((const char *)pathname, (const char *)name);
}

/* System call: lremovexattr */
long sys_lremovexattr(long pathname, long name, long unused1, long unused2, long unused3, long unused4) {
    /* Remove the extended attribute (don't follow symbolic links) */
    return file_lremovexattr((const char *)pathname, (const char *)name);
}

/* System call: fremovexattr */
long sys_fremovexattr(long fd, long name, long unused1, long unused2, long unused3, long unused4) {
    /* Get the file */
    file_t *file = process_get_file(task_current(), fd);
    
    if (file == NULL) {
        return -1;
    }
    
    /* Remove the extended attribute */
    return file_fremovexattr(file, (const char *)name);
}

/* Register extended attribute system calls */
void fs_xattr_syscalls_init(void) {
    /* Register extended attribute system calls */
    syscall_register(SYS_GETXATTR, sys_getxattr);
    syscall_register(SYS_LGETXATTR, sys_lgetxattr);
    syscall_register(SYS_FGETXATTR, sys_fgetxattr);
    syscall_register(SYS_SETXATTR, sys_setxattr);
    syscall_register(SYS_LSETXATTR, sys_lsetxattr);
    syscall_register(SYS_FSETXATTR, sys_fsetxattr);
    syscall_register(SYS_LISTXATTR, sys_listxattr);
    syscall_register(SYS_LLISTXATTR, sys_llistxattr);
    syscall_register(SYS_FLISTXATTR, sys_flistxattr);
    syscall_register(SYS_REMOVEXATTR, sys_removexattr);
    syscall_register(SYS_LREMOVEXATTR, sys_lremovexattr);
    syscall_register(SYS_FREMOVEXATTR, sys_fremovexattr);
}
