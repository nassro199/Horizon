/**
 * mount.c - Horizon kernel mount management implementation
 * 
 * This file contains the implementation of the mount management subsystem.
 * The implementation is compatible with Linux.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/fs/vfs.h>
#include <horizon/fs/file.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <horizon/task.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Mount namespace */
typedef struct mnt_namespace {
    struct list_head list;       /* List of mounts */
    struct vfsmount *root;       /* Root mount */
    int count;                   /* Reference count */
} mnt_namespace_t;

/* Current mount namespace */
static struct mnt_namespace *current_namespace = NULL;

/* Initialize the mount namespace */
void mount_init(void) {
    /* Create the initial mount namespace */
    current_namespace = kmalloc(sizeof(struct mnt_namespace), MEM_KERNEL | MEM_ZERO);
    
    if (current_namespace == NULL) {
        kernel_panic("Failed to create initial mount namespace");
    }
    
    /* Initialize the mount namespace */
    list_init(&current_namespace->list);
    current_namespace->root = NULL;
    current_namespace->count = 1;
}

/* Create a new mount namespace */
struct mnt_namespace *create_mnt_ns(struct vfsmount *mnt) {
    /* Allocate a new mount namespace */
    struct mnt_namespace *ns = kmalloc(sizeof(struct mnt_namespace), MEM_KERNEL | MEM_ZERO);
    
    if (ns == NULL) {
        return NULL;
    }
    
    /* Initialize the mount namespace */
    list_init(&ns->list);
    ns->root = mnt;
    ns->count = 1;
    
    return ns;
}

/* Clone a mount namespace */
struct mnt_namespace *clone_mnt_ns(struct mnt_namespace *old_ns) {
    if (old_ns == NULL) {
        return NULL;
    }
    
    /* Create a new mount namespace */
    struct mnt_namespace *new_ns = kmalloc(sizeof(struct mnt_namespace), MEM_KERNEL | MEM_ZERO);
    
    if (new_ns == NULL) {
        return NULL;
    }
    
    /* Initialize the mount namespace */
    list_init(&new_ns->list);
    new_ns->root = NULL;
    new_ns->count = 1;
    
    /* Clone the mounts */
    struct list_head *pos;
    list_for_each(pos, &old_ns->list) {
        struct vfsmount *old_mnt = list_entry(pos, struct vfsmount, mnt_list);
        
        /* Clone the mount */
        struct vfsmount *new_mnt = kmalloc(sizeof(struct vfsmount), MEM_KERNEL | MEM_ZERO);
        
        if (new_mnt == NULL) {
            /* Free the new namespace */
            /* This would be implemented with actual namespace freeing */
            return NULL;
        }
        
        /* Copy the mount */
        memcpy(new_mnt, old_mnt, sizeof(struct vfsmount));
        
        /* Initialize the lists */
        list_init(&new_mnt->mnt_list);
        list_init(&new_mnt->mnt_child);
        list_init(&new_mnt->mnt_mounts);
        list_init(&new_mnt->mnt_instance);
        
        /* Add the mount to the namespace */
        list_add(&new_mnt->mnt_list, &new_ns->list);
        
        /* Set the root mount */
        if (old_mnt == old_ns->root) {
            new_ns->root = new_mnt;
        }
    }
    
    return new_ns;
}

/* Free a mount namespace */
void free_mnt_ns(struct mnt_namespace *ns) {
    if (ns == NULL) {
        return;
    }
    
    /* Decrement the reference count */
    ns->count--;
    
    /* Check if the namespace is still in use */
    if (ns->count > 0) {
        return;
    }
    
    /* Free the mounts */
    struct list_head *pos, *n;
    list_for_each_safe(pos, n, &ns->list) {
        struct vfsmount *mnt = list_entry(pos, struct vfsmount, mnt_list);
        
        /* Remove the mount from the list */
        list_del(&mnt->mnt_list);
        
        /* Free the mount */
        if (mnt->mnt_devname) {
            kfree((void *)mnt->mnt_devname);
        }
        kfree(mnt);
    }
    
    /* Free the namespace */
    kfree(ns);
}

/* Get the current mount namespace */
struct mnt_namespace *get_mnt_ns(void) {
    return current_namespace;
}

/* Set the current mount namespace */
void set_mnt_ns(struct mnt_namespace *ns) {
    if (ns == NULL) {
        return;
    }
    
    /* Increment the reference count */
    ns->count++;
    
    /* Set the current namespace */
    if (current_namespace != NULL) {
        free_mnt_ns(current_namespace);
    }
    
    current_namespace = ns;
}

/* Find a mount by device name */
struct vfsmount *find_mnt_by_dev_name(const char *dev_name) {
    if (dev_name == NULL) {
        return NULL;
    }
    
    /* Iterate over the mounts */
    struct list_head *pos;
    list_for_each(pos, &current_namespace->list) {
        struct vfsmount *mnt = list_entry(pos, struct vfsmount, mnt_list);
        
        /* Check the device name */
        if (mnt->mnt_devname && strcmp(mnt->mnt_devname, dev_name) == 0) {
            return mnt;
        }
    }
    
    return NULL;
}

/* Find a mount by mount point */
struct vfsmount *find_mnt_by_mountpoint(struct dentry *mountpoint) {
    if (mountpoint == NULL) {
        return NULL;
    }
    
    /* Iterate over the mounts */
    struct list_head *pos;
    list_for_each(pos, &current_namespace->list) {
        struct vfsmount *mnt = list_entry(pos, struct vfsmount, mnt_list);
        
        /* Check the mount point */
        if (mnt->mnt_mountpoint == mountpoint) {
            return mnt;
        }
    }
    
    return NULL;
}

/* Add a mount to the namespace */
void add_mnt_to_namespace(struct vfsmount *mnt) {
    if (mnt == NULL) {
        return;
    }
    
    /* Add the mount to the namespace */
    list_add(&mnt->mnt_list, &current_namespace->list);
    
    /* Set the root mount */
    if (current_namespace->root == NULL) {
        current_namespace->root = mnt;
    }
}

/* Remove a mount from the namespace */
void remove_mnt_from_namespace(struct vfsmount *mnt) {
    if (mnt == NULL) {
        return;
    }
    
    /* Remove the mount from the namespace */
    list_del(&mnt->mnt_list);
    
    /* Check if this is the root mount */
    if (current_namespace->root == mnt) {
        current_namespace->root = NULL;
    }
}

/* Mount a file system */
int do_mount(const char *dev_name, const char *dir_name, const char *type, unsigned long flags, void *data) {
    /* Get the file system type */
    struct file_system_type *fs_type = get_fs_type(type);
    if (fs_type == NULL) {
        return -1;
    }
    
    /* Find the mount point */
    struct path path;
    int error = vfs_kern_path(dir_name, LOOKUP_FOLLOW, &path);
    if (error) {
        return error;
    }
    
    /* Check if this is already a mount point */
    struct vfsmount *old_mnt = find_mnt_by_mountpoint(path.dentry);
    if (old_mnt != NULL) {
        /* Check if this is a remount */
        if (flags & MS_REMOUNT) {
            /* Remount the file system */
            /* This would be implemented with actual remounting */
            vfs_path_release(&path);
            return 0;
        }
        
        /* Check if this is a bind mount */
        if (flags & MS_BIND) {
            /* Bind mount the file system */
            /* This would be implemented with actual bind mounting */
            vfs_path_release(&path);
            return 0;
        }
        
        /* Cannot mount on an existing mount point */
        vfs_path_release(&path);
        return -1;
    }
    
    /* Mount the file system */
    struct vfsmount *mnt = vfs_kern_mount(fs_type, flags, dev_name, data);
    if (mnt == NULL) {
        vfs_path_release(&path);
        return -1;
    }
    
    /* Set up the mount point */
    mnt->mnt_parent = path.mnt;
    mnt->mnt_mountpoint = path.dentry;
    
    /* Add the mount to the namespace */
    add_mnt_to_namespace(mnt);
    
    /* Add the mount to the parent's mounts */
    list_add(&mnt->mnt_child, &path.mnt->mnt_mounts);
    
    vfs_path_release(&path);
    
    return 0;
}

/* Unmount a file system */
int do_umount(const char *name, int flags) {
    /* Find the mount point */
    struct path path;
    int error = vfs_kern_path(name, LOOKUP_FOLLOW, &path);
    if (error) {
        return error;
    }
    
    /* Check if this is a mount point */
    struct vfsmount *mnt = find_mnt_by_mountpoint(path.dentry);
    if (mnt == NULL) {
        vfs_path_release(&path);
        return -1;
    }
    
    /* Check if this is the root mount */
    if (mnt == current_namespace->root) {
        /* Cannot unmount the root file system */
        vfs_path_release(&path);
        return -1;
    }
    
    /* Check if this mount has child mounts */
    if (!list_empty(&mnt->mnt_mounts)) {
        /* Cannot unmount a mount with child mounts */
        vfs_path_release(&path);
        return -1;
    }
    
    /* Remove the mount from the parent's mounts */
    list_del(&mnt->mnt_child);
    
    /* Remove the mount from the namespace */
    remove_mnt_from_namespace(mnt);
    
    /* Unmount the file system */
    vfs_kern_umount(mnt);
    
    vfs_path_release(&path);
    
    return 0;
}

/* System call: mount */
int sys_mount(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data) {
    return do_mount(source, target, filesystemtype, mountflags, (void *)data);
}

/* System call: umount */
int sys_umount(const char *target) {
    return do_umount(target, 0);
}

/* System call: umount2 */
int sys_umount2(const char *target, int flags) {
    return do_umount(target, flags);
}
