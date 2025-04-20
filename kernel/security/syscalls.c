/**
 * syscalls.c - Horizon kernel security system calls
 * 
 * This file contains the implementation of the security system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/security.h>
#include <horizon/task.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: getuid */
long sys_getuid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Get user ID */
    return security_getuid();
}

/* System call: geteuid */
long sys_geteuid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Get effective user ID */
    return security_geteuid();
}

/* System call: getgid */
long sys_getgid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Get group ID */
    return security_getgid();
}

/* System call: getegid */
long sys_getegid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Get effective group ID */
    return security_getegid();
}

/* System call: setuid */
long sys_setuid(long uid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Set user ID */
    return security_setuid(uid);
}

/* System call: setgid */
long sys_setgid(long gid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Set group ID */
    return security_setgid(gid);
}

/* System call: seteuid */
long sys_seteuid(long euid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Set effective user ID */
    return security_seteuid(euid);
}

/* System call: setegid */
long sys_setegid(long egid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Set effective group ID */
    return security_setegid(egid);
}

/* System call: setreuid */
long sys_setreuid(long ruid, long euid, long unused1, long unused2, long unused3, long unused4) {
    /* Set real and effective user IDs */
    return security_setreuid(ruid, euid);
}

/* System call: setregid */
long sys_setregid(long rgid, long egid, long unused1, long unused2, long unused3, long unused4) {
    /* Set real and effective group IDs */
    return security_setregid(rgid, egid);
}

/* System call: setresuid */
long sys_setresuid(long ruid, long euid, long suid, long unused1, long unused2, long unused3) {
    /* Set real, effective, and saved user IDs */
    return security_setresuid(ruid, euid, suid);
}

/* System call: getresuid */
long sys_getresuid(long ruid, long euid, long suid, long unused1, long unused2, long unused3) {
    /* Get real, effective, and saved user IDs */
    return security_getresuid((uid_t *)ruid, (uid_t *)euid, (uid_t *)suid);
}

/* System call: setresgid */
long sys_setresgid(long rgid, long egid, long sgid, long unused1, long unused2, long unused3) {
    /* Set real, effective, and saved group IDs */
    return security_setresgid(rgid, egid, sgid);
}

/* System call: getresgid */
long sys_getresgid(long rgid, long egid, long sgid, long unused1, long unused2, long unused3) {
    /* Get real, effective, and saved group IDs */
    return security_getresgid((gid_t *)rgid, (gid_t *)egid, (gid_t *)sgid);
}

/* System call: setfsuid */
long sys_setfsuid(long fsuid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Set file system user ID */
    return security_setfsuid(fsuid);
}

/* System call: setfsgid */
long sys_setfsgid(long fsgid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Set file system group ID */
    return security_setfsgid(fsgid);
}

/* System call: getgroups */
long sys_getgroups(long size, long list, long unused1, long unused2, long unused3, long unused4) {
    /* Get supplementary group IDs */
    return security_getgroups(size, (gid_t *)list);
}

/* System call: setgroups */
long sys_setgroups(long size, long list, long unused1, long unused2, long unused3, long unused4) {
    /* Set supplementary group IDs */
    return security_setgroups(size, (const gid_t *)list);
}

/* System call: capget */
long sys_capget(long header, long data, long unused1, long unused2, long unused3, long unused4) {
    /* Get capabilities */
    return security_capget((cap_user_header_t)header, (cap_user_data_t)data);
}

/* System call: capset */
long sys_capset(long header, long data, long unused1, long unused2, long unused3, long unused4) {
    /* Set capabilities */
    return security_capset((cap_user_header_t)header, (const cap_user_data_t)data);
}

/* System call: prctl */
long sys_prctl(long option, long arg2, long arg3, long arg4, long arg5, long unused1) {
    /* Operations on a process */
    return security_prctl(option, arg2, arg3, arg4, arg5);
}

/* System call: acct */
long sys_acct(long filename, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Switch process accounting on or off */
    return security_acct((const char *)filename);
}

/* System call: chroot */
long sys_chroot(long path, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Change root directory */
    return security_chroot((const char *)path);
}

/* System call: umask */
long sys_umask(long mask, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Set file mode creation mask */
    return security_umask(mask);
}

/* System call: mknod */
long sys_mknod(long pathname, long mode, long dev, long unused1, long unused2, long unused3) {
    /* Create a special or ordinary file */
    return security_mknod((const char *)pathname, mode, dev);
}

/* System call: chmod */
long sys_chmod(long pathname, long mode, long unused1, long unused2, long unused3, long unused4) {
    /* Change permissions of a file */
    return security_chmod((const char *)pathname, mode);
}

/* System call: chown */
long sys_chown(long pathname, long owner, long group, long unused1, long unused2, long unused3) {
    /* Change ownership of a file */
    return security_chown((const char *)pathname, owner, group);
}

/* System call: lchown */
long sys_lchown(long pathname, long owner, long group, long unused1, long unused2, long unused3) {
    /* Change ownership of a file (don't follow symbolic links) */
    return security_lchown((const char *)pathname, owner, group);
}

/* System call: fchown */
long sys_fchown(long fd, long owner, long group, long unused1, long unused2, long unused3) {
    /* Change ownership of a file */
    return security_fchown(fd, owner, group);
}

/* System call: fchmod */
long sys_fchmod(long fd, long mode, long unused1, long unused2, long unused3, long unused4) {
    /* Change permissions of a file */
    return security_fchmod(fd, mode);
}

/* System call: fchmodat */
long sys_fchmodat(long dirfd, long pathname, long mode, long flags, long unused1, long unused2) {
    /* Change permissions of a file relative to a directory file descriptor */
    return security_fchmodat(dirfd, (const char *)pathname, mode, flags);
}

/* System call: fchownat */
long sys_fchownat(long dirfd, long pathname, long owner, long group, long flags, long unused1) {
    /* Change ownership of a file relative to a directory file descriptor */
    return security_fchownat(dirfd, (const char *)pathname, owner, group, flags);
}

/* System call: faccessat */
long sys_faccessat(long dirfd, long pathname, long mode, long flags, long unused1, long unused2) {
    /* Check user's permissions for a file relative to a directory file descriptor */
    return security_faccessat(dirfd, (const char *)pathname, mode, flags);
}

/* System call: access */
long sys_access(long pathname, long mode, long unused1, long unused2, long unused3, long unused4) {
    /* Check user's permissions for a file */
    return security_access((const char *)pathname, mode);
}

/* Register security system calls */
void security_syscalls_init(void) {
    /* Register security system calls */
    syscall_register(SYS_GETUID, sys_getuid);
    syscall_register(SYS_GETEUID, sys_geteuid);
    syscall_register(SYS_GETGID, sys_getgid);
    syscall_register(SYS_GETEGID, sys_getegid);
    syscall_register(SYS_SETUID, sys_setuid);
    syscall_register(SYS_SETGID, sys_setgid);
    syscall_register(SYS_SETEUID, sys_seteuid);
    syscall_register(SYS_SETEGID, sys_setegid);
    syscall_register(SYS_SETREUID, sys_setreuid);
    syscall_register(SYS_SETREGID, sys_setregid);
    syscall_register(SYS_SETRESUID, sys_setresuid);
    syscall_register(SYS_GETRESUID, sys_getresuid);
    syscall_register(SYS_SETRESGID, sys_setresgid);
    syscall_register(SYS_GETRESGID, sys_getresgid);
    syscall_register(SYS_SETFSUID, sys_setfsuid);
    syscall_register(SYS_SETFSGID, sys_setfsgid);
    syscall_register(SYS_GETGROUPS, sys_getgroups);
    syscall_register(SYS_SETGROUPS, sys_setgroups);
    syscall_register(SYS_CAPGET, sys_capget);
    syscall_register(SYS_CAPSET, sys_capset);
    syscall_register(SYS_PRCTL, sys_prctl);
    syscall_register(SYS_ACCT, sys_acct);
    syscall_register(SYS_CHROOT, sys_chroot);
    syscall_register(SYS_UMASK, sys_umask);
    syscall_register(SYS_MKNOD, sys_mknod);
    syscall_register(SYS_CHMOD, sys_chmod);
    syscall_register(SYS_CHOWN, sys_chown);
    syscall_register(SYS_LCHOWN, sys_lchown);
    syscall_register(SYS_FCHOWN, sys_fchown);
    syscall_register(SYS_FCHMOD, sys_fchmod);
    syscall_register(SYS_FCHMODAT, sys_fchmodat);
    syscall_register(SYS_FCHOWNAT, sys_fchownat);
    syscall_register(SYS_FACCESSAT, sys_faccessat);
    syscall_register(SYS_ACCESS, sys_access);
}
