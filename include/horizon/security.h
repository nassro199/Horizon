/**
 * security.h - Security subsystem definitions
 *
 * This file contains definitions for the security subsystem.
 */

#ifndef _HORIZON_SECURITY_H
#define _HORIZON_SECURITY_H

#include <horizon/types.h>

/* Security context structure */
typedef struct security_context {
    u32 uid;                /* User ID */
    u32 gid;                /* Group ID */
    u32 euid;               /* Effective user ID */
    u32 egid;               /* Effective group ID */
    u32 suid;               /* Saved user ID */
    u32 sgid;               /* Saved group ID */
    u32 fsuid;              /* File system user ID */
    u32 fsgid;              /* File system group ID */
    u32 cap_inheritable;    /* Inheritable capabilities */
    u32 cap_permitted;      /* Permitted capabilities */
    u32 cap_effective;      /* Effective capabilities */
} security_context_t;

/* Capability bits */
#define CAP_CHOWN           0x00000001  /* Make arbitrary changes to file UIDs and GIDs */
#define CAP_DAC_OVERRIDE    0x00000002  /* Bypass file read, write, and execute permission checks */
#define CAP_DAC_READ_SEARCH 0x00000004  /* Bypass file read permission checks and directory read and execute permission checks */
#define CAP_FOWNER          0x00000008  /* Bypass permission checks on operations that normally require the file system UID of the process to match the UID of the file */
#define CAP_FSETID          0x00000010  /* Don't clear set-user-ID and set-group-ID permission bits when a file is modified */
#define CAP_KILL            0x00000020  /* Bypass permission checks for sending signals */
#define CAP_SETGID          0x00000040  /* Make arbitrary manipulations of process GIDs */
#define CAP_SETUID          0x00000080  /* Make arbitrary manipulations of process UIDs */
#define CAP_SETPCAP         0x00000100  /* If file capabilities are supported: add any capability from the calling thread's bounding set to its inheritable set; drop capabilities from the bounding set; make changes to the securebits flags */
#define CAP_SYS_ADMIN       0x00000200  /* Perform a range of system administration operations */
#define CAP_SYS_BOOT        0x00000400  /* Use reboot(2) and kexec_load(2) */
#define CAP_SYS_CHROOT      0x00000800  /* Use chroot(2) */
#define CAP_SYS_MODULE      0x00001000  /* Load and unload kernel modules */
#define CAP_SYS_NICE        0x00002000  /* Raise process nice value and change the nice value for arbitrary processes */
#define CAP_SYS_RESOURCE    0x00004000  /* Override resource limits */
#define CAP_SYS_TIME        0x00008000  /* Set system clock; set real-time (hardware) clock */
#define CAP_NET_ADMIN       0x00010000  /* Perform various network-related operations */
#define CAP_NET_BIND_SERVICE 0x00020000 /* Bind a socket to Internet domain privileged ports (port numbers less than 1024) */
#define CAP_NET_BROADCAST   0x00040000  /* Make socket broadcasts, and listen to multicasts */
#define CAP_NET_RAW         0x00080000  /* Use RAW and PACKET sockets */
#define CAP_IPC_LOCK        0x00100000  /* Lock memory */
#define CAP_IPC_OWNER       0x00200000  /* Bypass permission checks for operations on System V IPC objects */
#define CAP_SYS_PTRACE      0x00400000  /* Trace arbitrary processes using ptrace(2) */
#define CAP_SYS_PACCT       0x00800000  /* Use acct(2) */
#define CAP_MKNOD           0x01000000  /* Create special files using mknod(2) */
#define CAP_LEASE           0x02000000  /* Establish leases on arbitrary files */
#define CAP_AUDIT_WRITE     0x04000000  /* Write records to kernel auditing log */
#define CAP_AUDIT_CONTROL   0x08000000  /* Enable and disable kernel auditing */
#define CAP_SETFCAP         0x10000000  /* Set file capabilities */
#define CAP_MAC_OVERRIDE    0x20000000  /* Override Mandatory Access Control */
#define CAP_MAC_ADMIN       0x40000000  /* Allow MAC configuration or state changes */
#define CAP_ALL             0xFFFFFFFF  /* All capabilities */

/* Access control list entry structure */
typedef struct acl_entry {
    u32 tag;                /* Entry tag */
    u32 id;                 /* User or group ID */
    u32 perm;               /* Permissions */
    struct acl_entry *next; /* Next entry in list */
} acl_entry_t;

/* Access control list structure */
typedef struct acl {
    u32 count;              /* Number of entries */
    acl_entry_t *entries;   /* List of entries */
} acl_t;

/* ACL entry tags */
#define ACL_USER            1   /* User ACL entry */
#define ACL_GROUP           2   /* Group ACL entry */
#define ACL_OTHER           3   /* Other ACL entry */
#define ACL_MASK            4   /* Mask ACL entry */

/* ACL permissions */
#define ACL_READ            0x04    /* Read permission */
#define ACL_WRITE           0x02    /* Write permission */
#define ACL_EXECUTE         0x01    /* Execute permission */
#define ACL_ALL             0x07    /* All permissions */

/* Security operations structure */
typedef struct security_ops {
    int (*task_create)(struct security_context *parent, struct security_context *child);
    int (*task_setuid)(struct security_context *context, u32 uid);
    int (*task_setgid)(struct security_context *context, u32 gid);
    int (*task_kill)(struct security_context *context, u32 pid);
    int (*file_open)(struct security_context *context, const char *path, u32 flags);
    int (*file_permission)(struct security_context *context, const char *path, u32 mask);
    int (*file_chown)(struct security_context *context, const char *path, u32 uid, u32 gid);
    int (*file_chmod)(struct security_context *context, const char *path, u32 mode);
    int (*ipc_permission)(struct security_context *context, u32 key, u32 mask);
    int (*path_truncate)(const struct path *path);
    int (*path_mknod)(const struct path *dir, struct dentry *dentry, umode_t mode, unsigned int dev);
    int (*path_mkdir)(const struct path *dir, struct dentry *dentry, umode_t mode);
    int (*path_rmdir)(const struct path *dir, struct dentry *dentry);
    int (*path_unlink)(const struct path *dir, struct dentry *dentry);
    int (*path_symlink)(const struct path *dir, struct dentry *dentry, const char *old_name);
    int (*path_link)(struct dentry *old_dentry, const struct path *new_dir, struct dentry *new_dentry);
    int (*path_rename)(const struct path *old_dir, struct dentry *old_dentry, const struct path *new_dir, struct dentry *new_dentry, unsigned int flags);
    int (*path_chmod)(const struct path *path, umode_t mode);
    int (*path_chown)(const struct path *path, uid_t uid, gid_t gid);
    int (*path_chroot)(const struct path *path);
} security_ops_t;

/* Security module structure */
typedef struct security_module {
    char name[32];              /* Module name */
    security_ops_t *ops;        /* Security operations */
    struct security_module *next; /* Next module in list */
} security_module_t;

/* Security subsystem functions */
void security_init(void);
int security_register_module(security_module_t *module);
int security_unregister_module(security_module_t *module);
security_context_t *security_alloc_context(void);
void security_free_context(security_context_t *context);
int security_task_create(security_context_t *parent, security_context_t *child);
int security_task_setuid(security_context_t *context, u32 uid);
int security_task_setgid(security_context_t *context, u32 gid);
int security_task_kill(security_context_t *context, u32 pid);
int security_file_open(security_context_t *context, const char *path, u32 flags);
int security_file_permission(security_context_t *context, const char *path, u32 mask);
int security_file_chown(security_context_t *context, const char *path, u32 uid, u32 gid);
int security_file_chmod(security_context_t *context, const char *path, u32 mode);
int security_ipc_permission(security_context_t *context, u32 key, u32 mask);
int security_has_capability(security_context_t *context, u32 cap);
acl_t *security_acl_alloc(void);
void security_acl_free(acl_t *acl);
int security_acl_add_entry(acl_t *acl, u32 tag, u32 id, u32 perm);
int security_acl_remove_entry(acl_t *acl, u32 tag, u32 id);
int security_acl_check(acl_t *acl, security_context_t *context, u32 mask);

/* Path security functions */
int security_path_truncate(const struct path *path);
int security_path_mknod(const struct path *dir, struct dentry *dentry, umode_t mode, unsigned int dev);
int security_path_mkdir(const struct path *dir, struct dentry *dentry, umode_t mode);
int security_path_rmdir(const struct path *dir, struct dentry *dentry);
int security_path_unlink(const struct path *dir, struct dentry *dentry);
int security_path_symlink(const struct path *dir, struct dentry *dentry, const char *old_name);
int security_path_link(struct dentry *old_dentry, const struct path *new_dir, struct dentry *new_dentry);
int security_path_rename(const struct path *old_dir, struct dentry *old_dentry, const struct path *new_dir, struct dentry *new_dentry, unsigned int flags);
int security_path_chmod(const struct path *path, umode_t mode);
int security_path_chown(const struct path *path, uid_t uid, gid_t gid);
int security_path_chroot(const struct path *path);

#endif /* _KERNEL_SECURITY_H */
