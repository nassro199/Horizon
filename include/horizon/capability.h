/*
 * capability.h - Horizon kernel capability definitions
 *
 * This file contains definitions for process capabilities.
 */

#ifndef _HORIZON_CAPABILITY_H
#define _HORIZON_CAPABILITY_H

#include <horizon/types.h>

/* Capability bits */
#define CAP_CHOWN            0
#define CAP_DAC_OVERRIDE     1
#define CAP_DAC_READ_SEARCH  2
#define CAP_FOWNER           3
#define CAP_FSETID           4
#define CAP_KILL             5
#define CAP_SETGID           6
#define CAP_SETUID           7
#define CAP_SETPCAP          8
#define CAP_LINUX_IMMUTABLE  9
#define CAP_NET_BIND_SERVICE 10
#define CAP_NET_BROADCAST    11
#define CAP_NET_ADMIN        12
#define CAP_NET_RAW          13
#define CAP_IPC_LOCK         14
#define CAP_IPC_OWNER        15
#define CAP_SYS_MODULE       16
#define CAP_SYS_RAWIO        17
#define CAP_SYS_CHROOT       18
#define CAP_SYS_PTRACE       19
#define CAP_SYS_PACCT        20
#define CAP_SYS_ADMIN        21
#define CAP_SYS_BOOT         22
#define CAP_SYS_NICE         23
#define CAP_SYS_RESOURCE     24
#define CAP_SYS_TIME         25
#define CAP_SYS_TTY_CONFIG   26
#define CAP_MKNOD            27
#define CAP_LEASE            28
#define CAP_AUDIT_WRITE      29
#define CAP_AUDIT_CONTROL    30
#define CAP_SETFCAP          31
#define CAP_MAC_OVERRIDE     32
#define CAP_MAC_ADMIN        33
#define CAP_SYSLOG           34
#define CAP_WAKE_ALARM       35
#define CAP_BLOCK_SUSPEND    36
#define CAP_AUDIT_READ       37
#define CAP_LAST_CAP         CAP_AUDIT_READ

/* Capability set size */
#define _KERNEL_CAPABILITY_U32S  (_KERNEL_CAPABILITY_BITS + 31) / 32

/* Capability set */
typedef struct kernel_cap_struct {
    __u32 cap[_KERNEL_CAPABILITY_U32S];
} kernel_cap_t;

/* Capability functions */
extern int capable(int cap);
extern int ns_capable(struct user_namespace *ns, int cap);
extern int file_ns_capable(const struct file *file, struct user_namespace *ns, int cap);
extern int has_capability(struct task_struct *t, int cap);
extern int has_ns_capability(struct task_struct *t, struct user_namespace *ns, int cap);
extern int has_file_ns_capability(struct task_struct *t, struct user_namespace *ns, const struct file *file, int cap);

#endif /* _HORIZON_CAPABILITY_H */
