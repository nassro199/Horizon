/**
 * security.c - Security subsystem implementation
 * 
 * This file contains the implementation of the security subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/mm.h>
#include <horizon/security.h>
#include <horizon/string.h>

/* Security module list */
static security_module_t *security_modules = NULL;

/* Default security operations */
static int default_task_create(security_context_t *parent, security_context_t *child)
{
    if (parent == NULL || child == NULL) {
        return -1;
    }
    
    /* Copy the parent's security context to the child */
    child->uid = parent->uid;
    child->gid = parent->gid;
    child->euid = parent->euid;
    child->egid = parent->egid;
    child->suid = parent->suid;
    child->sgid = parent->sgid;
    child->fsuid = parent->fsuid;
    child->fsgid = parent->fsgid;
    child->cap_inheritable = parent->cap_inheritable;
    child->cap_permitted = parent->cap_permitted;
    child->cap_effective = parent->cap_effective;
    
    return 0;
}

static int default_task_setuid(security_context_t *context, u32 uid)
{
    if (context == NULL) {
        return -1;
    }
    
    /* Check if the process has the capability to set the UID */
    if (context->euid != 0 && !security_has_capability(context, CAP_SETUID)) {
        return -1;
    }
    
    /* Set the UID */
    context->uid = uid;
    context->euid = uid;
    context->suid = uid;
    context->fsuid = uid;
    
    return 0;
}

static int default_task_setgid(security_context_t *context, u32 gid)
{
    if (context == NULL) {
        return -1;
    }
    
    /* Check if the process has the capability to set the GID */
    if (context->euid != 0 && !security_has_capability(context, CAP_SETGID)) {
        return -1;
    }
    
    /* Set the GID */
    context->gid = gid;
    context->egid = gid;
    context->sgid = gid;
    context->fsgid = gid;
    
    return 0;
}

static int default_task_kill(security_context_t *context, u32 pid)
{
    if (context == NULL) {
        return -1;
    }
    
    /* This would be implemented with actual process lookup */
    /* For now, just check if the process has the capability to kill */
    if (context->euid != 0 && !security_has_capability(context, CAP_KILL)) {
        return -1;
    }
    
    return 0;
}

static int default_file_open(security_context_t *context, const char *path, u32 flags)
{
    if (context == NULL || path == NULL) {
        return -1;
    }
    
    /* This would be implemented with actual file permission checking */
    /* For now, just return success */
    return 0;
}

static int default_file_permission(security_context_t *context, const char *path, u32 mask)
{
    if (context == NULL || path == NULL) {
        return -1;
    }
    
    /* This would be implemented with actual file permission checking */
    /* For now, just return success */
    return 0;
}

static int default_file_chown(security_context_t *context, const char *path, u32 uid, u32 gid)
{
    if (context == NULL || path == NULL) {
        return -1;
    }
    
    /* Check if the process has the capability to change ownership */
    if (context->euid != 0 && !security_has_capability(context, CAP_CHOWN)) {
        return -1;
    }
    
    return 0;
}

static int default_file_chmod(security_context_t *context, const char *path, u32 mode)
{
    if (context == NULL || path == NULL) {
        return -1;
    }
    
    /* This would be implemented with actual file permission checking */
    /* For now, just return success */
    return 0;
}

static int default_ipc_permission(security_context_t *context, u32 key, u32 mask)
{
    if (context == NULL) {
        return -1;
    }
    
    /* This would be implemented with actual IPC permission checking */
    /* For now, just return success */
    return 0;
}

/* Default security operations */
static security_ops_t default_security_ops = {
    .task_create = default_task_create,
    .task_setuid = default_task_setuid,
    .task_setgid = default_task_setgid,
    .task_kill = default_task_kill,
    .file_open = default_file_open,
    .file_permission = default_file_permission,
    .file_chown = default_file_chown,
    .file_chmod = default_file_chmod,
    .ipc_permission = default_ipc_permission
};

/* Default security module */
static security_module_t default_security_module = {
    .name = "default",
    .ops = &default_security_ops,
    .next = NULL
};

/* Initialize the security subsystem */
void security_init(void)
{
    /* Initialize the security module list */
    security_modules = NULL;
    
    /* Register the default security module */
    security_register_module(&default_security_module);
}

/* Register a security module */
int security_register_module(security_module_t *module)
{
    if (module == NULL || module->ops == NULL) {
        return -1;
    }
    
    /* Add to the security module list */
    module->next = security_modules;
    security_modules = module;
    
    return 0;
}

/* Unregister a security module */
int security_unregister_module(security_module_t *module)
{
    if (module == NULL) {
        return -1;
    }
    
    /* Don't allow unregistering the default module */
    if (module == &default_security_module) {
        return -1;
    }
    
    /* Remove from the security module list */
    if (security_modules == module) {
        security_modules = module->next;
    } else {
        security_module_t *prev = security_modules;
        
        while (prev != NULL && prev->next != module) {
            prev = prev->next;
        }
        
        if (prev != NULL) {
            prev->next = module->next;
        }
    }
    
    return 0;
}

/* Allocate a security context */
security_context_t *security_alloc_context(void)
{
    /* Allocate a security context */
    security_context_t *context = kmalloc(sizeof(security_context_t), MEM_KERNEL | MEM_ZERO);
    
    if (context == NULL) {
        return NULL;
    }
    
    /* Initialize the security context */
    context->uid = 0;
    context->gid = 0;
    context->euid = 0;
    context->egid = 0;
    context->suid = 0;
    context->sgid = 0;
    context->fsuid = 0;
    context->fsgid = 0;
    context->cap_inheritable = 0;
    context->cap_permitted = CAP_ALL;
    context->cap_effective = CAP_ALL;
    
    return context;
}

/* Free a security context */
void security_free_context(security_context_t *context)
{
    if (context == NULL) {
        return;
    }
    
    /* Free the security context */
    kfree(context);
}

/* Create a task */
int security_task_create(security_context_t *parent, security_context_t *child)
{
    if (parent == NULL || child == NULL) {
        return -1;
    }
    
    /* Call each security module's task_create function */
    security_module_t *module = security_modules;
    
    while (module != NULL) {
        if (module->ops != NULL && module->ops->task_create != NULL) {
            int result = module->ops->task_create(parent, child);
            
            if (result < 0) {
                return result;
            }
        }
        
        module = module->next;
    }
    
    return 0;
}

/* Set the UID of a task */
int security_task_setuid(security_context_t *context, u32 uid)
{
    if (context == NULL) {
        return -1;
    }
    
    /* Call each security module's task_setuid function */
    security_module_t *module = security_modules;
    
    while (module != NULL) {
        if (module->ops != NULL && module->ops->task_setuid != NULL) {
            int result = module->ops->task_setuid(context, uid);
            
            if (result < 0) {
                return result;
            }
        }
        
        module = module->next;
    }
    
    return 0;
}

/* Set the GID of a task */
int security_task_setgid(security_context_t *context, u32 gid)
{
    if (context == NULL) {
        return -1;
    }
    
    /* Call each security module's task_setgid function */
    security_module_t *module = security_modules;
    
    while (module != NULL) {
        if (module->ops != NULL && module->ops->task_setgid != NULL) {
            int result = module->ops->task_setgid(context, gid);
            
            if (result < 0) {
                return result;
            }
        }
        
        module = module->next;
    }
    
    return 0;
}

/* Kill a task */
int security_task_kill(security_context_t *context, u32 pid)
{
    if (context == NULL) {
        return -1;
    }
    
    /* Call each security module's task_kill function */
    security_module_t *module = security_modules;
    
    while (module != NULL) {
        if (module->ops != NULL && module->ops->task_kill != NULL) {
            int result = module->ops->task_kill(context, pid);
            
            if (result < 0) {
                return result;
            }
        }
        
        module = module->next;
    }
    
    return 0;
}

/* Open a file */
int security_file_open(security_context_t *context, const char *path, u32 flags)
{
    if (context == NULL || path == NULL) {
        return -1;
    }
    
    /* Call each security module's file_open function */
    security_module_t *module = security_modules;
    
    while (module != NULL) {
        if (module->ops != NULL && module->ops->file_open != NULL) {
            int result = module->ops->file_open(context, path, flags);
            
            if (result < 0) {
                return result;
            }
        }
        
        module = module->next;
    }
    
    return 0;
}

/* Check file permissions */
int security_file_permission(security_context_t *context, const char *path, u32 mask)
{
    if (context == NULL || path == NULL) {
        return -1;
    }
    
    /* Call each security module's file_permission function */
    security_module_t *module = security_modules;
    
    while (module != NULL) {
        if (module->ops != NULL && module->ops->file_permission != NULL) {
            int result = module->ops->file_permission(context, path, mask);
            
            if (result < 0) {
                return result;
            }
        }
        
        module = module->next;
    }
    
    return 0;
}

/* Change file ownership */
int security_file_chown(security_context_t *context, const char *path, u32 uid, u32 gid)
{
    if (context == NULL || path == NULL) {
        return -1;
    }
    
    /* Call each security module's file_chown function */
    security_module_t *module = security_modules;
    
    while (module != NULL) {
        if (module->ops != NULL && module->ops->file_chown != NULL) {
            int result = module->ops->file_chown(context, path, uid, gid);
            
            if (result < 0) {
                return result;
            }
        }
        
        module = module->next;
    }
    
    return 0;
}

/* Change file permissions */
int security_file_chmod(security_context_t *context, const char *path, u32 mode)
{
    if (context == NULL || path == NULL) {
        return -1;
    }
    
    /* Call each security module's file_chmod function */
    security_module_t *module = security_modules;
    
    while (module != NULL) {
        if (module->ops != NULL && module->ops->file_chmod != NULL) {
            int result = module->ops->file_chmod(context, path, mode);
            
            if (result < 0) {
                return result;
            }
        }
        
        module = module->next;
    }
    
    return 0;
}

/* Check IPC permissions */
int security_ipc_permission(security_context_t *context, u32 key, u32 mask)
{
    if (context == NULL) {
        return -1;
    }
    
    /* Call each security module's ipc_permission function */
    security_module_t *module = security_modules;
    
    while (module != NULL) {
        if (module->ops != NULL && module->ops->ipc_permission != NULL) {
            int result = module->ops->ipc_permission(context, key, mask);
            
            if (result < 0) {
                return result;
            }
        }
        
        module = module->next;
    }
    
    return 0;
}

/* Check if a security context has a capability */
int security_has_capability(security_context_t *context, u32 cap)
{
    if (context == NULL) {
        return 0;
    }
    
    /* Check if the capability is in the effective set */
    return (context->cap_effective & cap) != 0;
}

/* Allocate an ACL */
acl_t *security_acl_alloc(void)
{
    /* Allocate an ACL */
    acl_t *acl = kmalloc(sizeof(acl_t), MEM_KERNEL | MEM_ZERO);
    
    if (acl == NULL) {
        return NULL;
    }
    
    /* Initialize the ACL */
    acl->count = 0;
    acl->entries = NULL;
    
    return acl;
}

/* Free an ACL */
void security_acl_free(acl_t *acl)
{
    if (acl == NULL) {
        return;
    }
    
    /* Free all entries */
    acl_entry_t *entry = acl->entries;
    
    while (entry != NULL) {
        acl_entry_t *next = entry->next;
        kfree(entry);
        entry = next;
    }
    
    /* Free the ACL */
    kfree(acl);
}

/* Add an entry to an ACL */
int security_acl_add_entry(acl_t *acl, u32 tag, u32 id, u32 perm)
{
    if (acl == NULL) {
        return -1;
    }
    
    /* Allocate an entry */
    acl_entry_t *entry = kmalloc(sizeof(acl_entry_t), MEM_KERNEL | MEM_ZERO);
    
    if (entry == NULL) {
        return -1;
    }
    
    /* Initialize the entry */
    entry->tag = tag;
    entry->id = id;
    entry->perm = perm;
    entry->next = NULL;
    
    /* Add to the ACL */
    if (acl->entries == NULL) {
        acl->entries = entry;
    } else {
        acl_entry_t *last = acl->entries;
        
        while (last->next != NULL) {
            last = last->next;
        }
        
        last->next = entry;
    }
    
    /* Increment the count */
    acl->count++;
    
    return 0;
}

/* Remove an entry from an ACL */
int security_acl_remove_entry(acl_t *acl, u32 tag, u32 id)
{
    if (acl == NULL) {
        return -1;
    }
    
    /* Find the entry */
    acl_entry_t *entry = acl->entries;
    acl_entry_t *prev = NULL;
    
    while (entry != NULL) {
        if (entry->tag == tag && entry->id == id) {
            /* Found the entry */
            if (prev == NULL) {
                /* First entry */
                acl->entries = entry->next;
            } else {
                /* Not the first entry */
                prev->next = entry->next;
            }
            
            /* Free the entry */
            kfree(entry);
            
            /* Decrement the count */
            acl->count--;
            
            return 0;
        }
        
        prev = entry;
        entry = entry->next;
    }
    
    /* Entry not found */
    return -1;
}

/* Check if an ACL allows access */
int security_acl_check(acl_t *acl, security_context_t *context, u32 mask)
{
    if (acl == NULL || context == NULL) {
        return -1;
    }
    
    /* Check if the user is root */
    if (context->euid == 0) {
        return 0;
    }
    
    /* Find the user entry */
    acl_entry_t *entry = acl->entries;
    acl_entry_t *user_entry = NULL;
    acl_entry_t *group_entry = NULL;
    acl_entry_t *other_entry = NULL;
    acl_entry_t *mask_entry = NULL;
    
    while (entry != NULL) {
        if (entry->tag == ACL_USER && entry->id == context->euid) {
            user_entry = entry;
        } else if (entry->tag == ACL_GROUP && entry->id == context->egid) {
            group_entry = entry;
        } else if (entry->tag == ACL_OTHER) {
            other_entry = entry;
        } else if (entry->tag == ACL_MASK) {
            mask_entry = entry;
        }
        
        entry = entry->next;
    }
    
    /* Check permissions */
    if (user_entry != NULL) {
        /* User entry exists */
        if (mask_entry != NULL) {
            /* Apply the mask */
            return (user_entry->perm & mask_entry->perm & mask) == mask ? 0 : -1;
        } else {
            /* No mask */
            return (user_entry->perm & mask) == mask ? 0 : -1;
        }
    } else if (group_entry != NULL) {
        /* Group entry exists */
        if (mask_entry != NULL) {
            /* Apply the mask */
            return (group_entry->perm & mask_entry->perm & mask) == mask ? 0 : -1;
        } else {
            /* No mask */
            return (group_entry->perm & mask) == mask ? 0 : -1;
        }
    } else if (other_entry != NULL) {
        /* Other entry exists */
        return (other_entry->perm & mask) == mask ? 0 : -1;
    }
    
    /* No matching entry */
    return -1;
}
