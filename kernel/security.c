/**
 * security.c - Horizon kernel security implementation
 * 
 * This file contains the implementation of the security subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/security.h>
#include <horizon/spinlock.h>
#include <horizon/list.h>
#include <horizon/mm.h>
#include <horizon/errno.h>
#include <horizon/string.h>

/* Security module list */
static security_module_t *security_modules = NULL;

/* Security lock */
static spinlock_t security_lock = SPIN_LOCK_INITIALIZER;

/**
 * Initialize security subsystem
 */
void security_init(void) {
    /* Initialize lock */
    spin_lock_init(&security_lock, "security");
}

/**
 * Register a security module
 * 
 * @param module Module to register
 * @return 0 on success, negative error code on failure
 */
int security_register_module(security_module_t *module) {
    security_module_t *curr, *prev;
    
    /* Check parameters */
    if (module == NULL || module->name[0] == '\0' || module->ops == NULL) {
        return -EINVAL;
    }
    
    /* Register module */
    spin_lock(&security_lock);
    
    /* Check if module already exists */
    curr = security_modules;
    while (curr != NULL) {
        if (strcmp(curr->name, module->name) == 0) {
            spin_unlock(&security_lock);
            return -EEXIST;
        }
        curr = curr->next;
    }
    
    /* Add module to list */
    if (security_modules == NULL) {
        /* First module */
        security_modules = module;
        module->next = NULL;
    } else {
        /* Add to end of list */
        curr = security_modules;
        prev = NULL;
        while (curr != NULL) {
            prev = curr;
            curr = curr->next;
        }
        prev->next = module;
        module->next = NULL;
    }
    
    spin_unlock(&security_lock);
    
    return 0;
}

/**
 * Unregister a security module
 * 
 * @param module Module to unregister
 * @return 0 on success, negative error code on failure
 */
int security_unregister_module(security_module_t *module) {
    security_module_t *curr, *prev;
    
    /* Check parameters */
    if (module == NULL) {
        return -EINVAL;
    }
    
    /* Unregister module */
    spin_lock(&security_lock);
    
    /* Find module in list */
    curr = security_modules;
    prev = NULL;
    while (curr != NULL) {
        if (curr == module) {
            /* Remove from list */
            if (prev == NULL) {
                security_modules = curr->next;
            } else {
                prev->next = curr->next;
            }
            spin_unlock(&security_lock);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    
    spin_unlock(&security_lock);
    
    /* Module not found */
    return -ENOENT;
}

/**
 * Allocate a security context
 * 
 * @return Security context, or NULL on failure
 */
security_context_t *security_alloc_context(void) {
    security_context_t *context;
    
    /* Allocate context */
    context = kmalloc(sizeof(security_context_t), MEM_KERNEL | MEM_ZERO);
    if (context == NULL) {
        return NULL;
    }
    
    return context;
}

/**
 * Free a security context
 * 
 * @param context Context to free
 */
void security_free_context(security_context_t *context) {
    /* Check parameters */
    if (context == NULL) {
        return;
    }
    
    /* Free context */
    kfree(context);
}

/**
 * Create a task security context
 * 
 * @param parent Parent context
 * @param child Child context
 * @return 0 on success, negative error code on failure
 */
int security_task_create(security_context_t *parent, security_context_t *child) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (parent == NULL || child == NULL) {
        return -EINVAL;
    }
    
    /* Copy parent context to child */
    memcpy(child, parent, sizeof(security_context_t));
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->task_create != NULL) {
            ret = module->ops->task_create(parent, child);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}

/**
 * Set task user ID
 * 
 * @param context Security context
 * @param uid User ID
 * @return 0 on success, negative error code on failure
 */
int security_task_setuid(security_context_t *context, u32 uid) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (context == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->task_setuid != NULL) {
            ret = module->ops->task_setuid(context, uid);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    /* Set user ID */
    if (ret == 0) {
        context->uid = uid;
    }
    
    return ret;
}

/**
 * Set task group ID
 * 
 * @param context Security context
 * @param gid Group ID
 * @return 0 on success, negative error code on failure
 */
int security_task_setgid(security_context_t *context, u32 gid) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (context == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->task_setgid != NULL) {
            ret = module->ops->task_setgid(context, gid);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    /* Set group ID */
    if (ret == 0) {
        context->gid = gid;
    }
    
    return ret;
}

/**
 * Check if task can kill another task
 * 
 * @param context Security context
 * @param pid Process ID
 * @return 0 on success, negative error code on failure
 */
int security_task_kill(security_context_t *context, u32 pid) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (context == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->task_kill != NULL) {
            ret = module->ops->task_kill(context, pid);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}

/**
 * Check if task can open a file
 * 
 * @param context Security context
 * @param path File path
 * @param flags Open flags
 * @return 0 on success, negative error code on failure
 */
int security_file_open(security_context_t *context, const char *path, u32 flags) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (context == NULL || path == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->file_open != NULL) {
            ret = module->ops->file_open(context, path, flags);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}

/**
 * Check if task has permission to access a file
 * 
 * @param context Security context
 * @param path File path
 * @param mask Permission mask
 * @return 0 on success, negative error code on failure
 */
int security_file_permission(security_context_t *context, const char *path, u32 mask) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (context == NULL || path == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->file_permission != NULL) {
            ret = module->ops->file_permission(context, path, mask);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}

/**
 * Check if task can change file ownership
 * 
 * @param context Security context
 * @param path File path
 * @param uid User ID
 * @param gid Group ID
 * @return 0 on success, negative error code on failure
 */
int security_file_chown(security_context_t *context, const char *path, u32 uid, u32 gid) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (context == NULL || path == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->file_chown != NULL) {
            ret = module->ops->file_chown(context, path, uid, gid);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}

/**
 * Check if task can change file mode
 * 
 * @param context Security context
 * @param path File path
 * @param mode File mode
 * @return 0 on success, negative error code on failure
 */
int security_file_chmod(security_context_t *context, const char *path, u32 mode) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (context == NULL || path == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->file_chmod != NULL) {
            ret = module->ops->file_chmod(context, path, mode);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}

/**
 * Check if task has permission to access IPC
 * 
 * @param context Security context
 * @param key IPC key
 * @param mask Permission mask
 * @return 0 on success, negative error code on failure
 */
int security_ipc_permission(security_context_t *context, u32 key, u32 mask) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (context == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->ipc_permission != NULL) {
            ret = module->ops->ipc_permission(context, key, mask);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}

/**
 * Check if task has a capability
 * 
 * @param context Security context
 * @param cap Capability
 * @return 1 if task has capability, 0 if not
 */
int security_has_capability(security_context_t *context, u32 cap) {
    /* Check parameters */
    if (context == NULL) {
        return 0;
    }
    
    /* Check if task has capability */
    return (context->cap_effective & cap) != 0;
}

/**
 * Allocate an ACL
 * 
 * @return ACL, or NULL on failure
 */
acl_t *security_acl_alloc(void) {
    acl_t *acl;
    
    /* Allocate ACL */
    acl = kmalloc(sizeof(acl_t), MEM_KERNEL | MEM_ZERO);
    if (acl == NULL) {
        return NULL;
    }
    
    return acl;
}

/**
 * Free an ACL
 * 
 * @param acl ACL to free
 */
void security_acl_free(acl_t *acl) {
    acl_entry_t *entry, *next;
    
    /* Check parameters */
    if (acl == NULL) {
        return;
    }
    
    /* Free entries */
    entry = acl->entries;
    while (entry != NULL) {
        next = entry->next;
        kfree(entry);
        entry = next;
    }
    
    /* Free ACL */
    kfree(acl);
}

/**
 * Add an entry to an ACL
 * 
 * @param acl ACL to add to
 * @param tag Entry tag
 * @param id User or group ID
 * @param perm Permissions
 * @return 0 on success, negative error code on failure
 */
int security_acl_add_entry(acl_t *acl, u32 tag, u32 id, u32 perm) {
    acl_entry_t *entry;
    
    /* Check parameters */
    if (acl == NULL) {
        return -EINVAL;
    }
    
    /* Allocate entry */
    entry = kmalloc(sizeof(acl_entry_t), MEM_KERNEL);
    if (entry == NULL) {
        return -ENOMEM;
    }
    
    /* Initialize entry */
    entry->tag = tag;
    entry->id = id;
    entry->perm = perm;
    
    /* Add to list */
    entry->next = acl->entries;
    acl->entries = entry;
    acl->count++;
    
    return 0;
}

/**
 * Remove an entry from an ACL
 * 
 * @param acl ACL to remove from
 * @param tag Entry tag
 * @param id User or group ID
 * @return 0 on success, negative error code on failure
 */
int security_acl_remove_entry(acl_t *acl, u32 tag, u32 id) {
    acl_entry_t *entry, *prev;
    
    /* Check parameters */
    if (acl == NULL) {
        return -EINVAL;
    }
    
    /* Find entry */
    entry = acl->entries;
    prev = NULL;
    while (entry != NULL) {
        if (entry->tag == tag && entry->id == id) {
            /* Remove from list */
            if (prev == NULL) {
                acl->entries = entry->next;
            } else {
                prev->next = entry->next;
            }
            
            /* Free entry */
            kfree(entry);
            acl->count--;
            
            return 0;
        }
        prev = entry;
        entry = entry->next;
    }
    
    /* Entry not found */
    return -ENOENT;
}

/**
 * Check if a task has permission to access an object with an ACL
 * 
 * @param acl ACL to check
 * @param context Security context
 * @param mask Permission mask
 * @return 0 on success, negative error code on failure
 */
int security_acl_check(acl_t *acl, security_context_t *context, u32 mask) {
    acl_entry_t *entry;
    u32 perm = 0;
    
    /* Check parameters */
    if (acl == NULL || context == NULL) {
        return -EINVAL;
    }
    
    /* Check if task is root */
    if (context->uid == 0) {
        return 0;
    }
    
    /* Check ACL entries */
    entry = acl->entries;
    while (entry != NULL) {
        if (entry->tag == ACL_USER && entry->id == context->uid) {
            /* User entry */
            perm = entry->perm;
            break;
        } else if (entry->tag == ACL_GROUP && entry->id == context->gid) {
            /* Group entry */
            perm = entry->perm;
            break;
        } else if (entry->tag == ACL_OTHER) {
            /* Other entry */
            perm = entry->perm;
            break;
        }
        entry = entry->next;
    }
    
    /* Check permissions */
    if ((perm & mask) != mask) {
        return -EACCES;
    }
    
    return 0;
}

/**
 * Check if a task can truncate a file
 * 
 * @param path File path
 * @return 0 on success, negative error code on failure
 */
int security_path_truncate(const struct path *path) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (path == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->path_truncate != NULL) {
            ret = module->ops->path_truncate(path);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}

/**
 * Check if a task can create a special file
 * 
 * @param dir Directory path
 * @param dentry Directory entry
 * @param mode File mode
 * @param dev Device number
 * @return 0 on success, negative error code on failure
 */
int security_path_mknod(const struct path *dir, struct dentry *dentry, umode_t mode, unsigned int dev) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (dir == NULL || dentry == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->path_mknod != NULL) {
            ret = module->ops->path_mknod(dir, dentry, mode, dev);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}

/**
 * Check if a task can create a directory
 * 
 * @param dir Directory path
 * @param dentry Directory entry
 * @param mode Directory mode
 * @return 0 on success, negative error code on failure
 */
int security_path_mkdir(const struct path *dir, struct dentry *dentry, umode_t mode) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (dir == NULL || dentry == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->path_mkdir != NULL) {
            ret = module->ops->path_mkdir(dir, dentry, mode);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}

/**
 * Check if a task can remove a directory
 * 
 * @param dir Directory path
 * @param dentry Directory entry
 * @return 0 on success, negative error code on failure
 */
int security_path_rmdir(const struct path *dir, struct dentry *dentry) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (dir == NULL || dentry == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->path_rmdir != NULL) {
            ret = module->ops->path_rmdir(dir, dentry);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}

/**
 * Check if a task can unlink a file
 * 
 * @param dir Directory path
 * @param dentry Directory entry
 * @return 0 on success, negative error code on failure
 */
int security_path_unlink(const struct path *dir, struct dentry *dentry) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (dir == NULL || dentry == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->path_unlink != NULL) {
            ret = module->ops->path_unlink(dir, dentry);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}

/**
 * Check if a task can create a symbolic link
 * 
 * @param dir Directory path
 * @param dentry Directory entry
 * @param old_name Target name
 * @return 0 on success, negative error code on failure
 */
int security_path_symlink(const struct path *dir, struct dentry *dentry, const char *old_name) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (dir == NULL || dentry == NULL || old_name == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->path_symlink != NULL) {
            ret = module->ops->path_symlink(dir, dentry, old_name);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}

/**
 * Check if a task can create a hard link
 * 
 * @param old_dentry Old directory entry
 * @param new_dir New directory path
 * @param new_dentry New directory entry
 * @return 0 on success, negative error code on failure
 */
int security_path_link(struct dentry *old_dentry, const struct path *new_dir, struct dentry *new_dentry) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (old_dentry == NULL || new_dir == NULL || new_dentry == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->path_link != NULL) {
            ret = module->ops->path_link(old_dentry, new_dir, new_dentry);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}

/**
 * Check if a task can rename a file
 * 
 * @param old_dir Old directory path
 * @param old_dentry Old directory entry
 * @param new_dir New directory path
 * @param new_dentry New directory entry
 * @param flags Rename flags
 * @return 0 on success, negative error code on failure
 */
int security_path_rename(const struct path *old_dir, struct dentry *old_dentry, const struct path *new_dir, struct dentry *new_dentry, unsigned int flags) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (old_dir == NULL || old_dentry == NULL || new_dir == NULL || new_dentry == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->path_rename != NULL) {
            ret = module->ops->path_rename(old_dir, old_dentry, new_dir, new_dentry, flags);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}

/**
 * Check if a task can change file mode
 * 
 * @param path File path
 * @param mode File mode
 * @return 0 on success, negative error code on failure
 */
int security_path_chmod(const struct path *path, umode_t mode) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (path == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->path_chmod != NULL) {
            ret = module->ops->path_chmod(path, mode);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}

/**
 * Check if a task can change file ownership
 * 
 * @param path File path
 * @param uid User ID
 * @param gid Group ID
 * @return 0 on success, negative error code on failure
 */
int security_path_chown(const struct path *path, uid_t uid, gid_t gid) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (path == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->path_chown != NULL) {
            ret = module->ops->path_chown(path, uid, gid);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}

/**
 * Check if a task can change root directory
 * 
 * @param path File path
 * @return 0 on success, negative error code on failure
 */
int security_path_chroot(const struct path *path) {
    security_module_t *module;
    int ret = 0;
    
    /* Check parameters */
    if (path == NULL) {
        return -EINVAL;
    }
    
    /* Call security modules */
    spin_lock(&security_lock);
    module = security_modules;
    while (module != NULL) {
        if (module->ops != NULL && module->ops->path_chroot != NULL) {
            ret = module->ops->path_chroot(path);
            if (ret != 0) {
                break;
            }
        }
        module = module->next;
    }
    spin_unlock(&security_lock);
    
    return ret;
}
