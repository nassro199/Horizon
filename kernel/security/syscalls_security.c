/**
 * syscalls_security.c - Horizon kernel security system calls
 * 
 * This file contains the implementation of the security system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/security.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System call: capget */
long sys_capget(long header, long data, long unused1, long unused2, long unused3, long unused4) {
    /* Get the capabilities */
    return capability_get(((struct __user_cap_header_struct *)header)->pid, (struct __user_cap_header_struct *)header, (struct __user_cap_data_struct *)data);
}

/* System call: capset */
long sys_capset(long header, long data, long unused1, long unused2, long unused3, long unused4) {
    /* Set the capabilities */
    return capability_set(((struct __user_cap_header_struct *)header)->pid, (struct __user_cap_header_struct *)header, (struct __user_cap_data_struct *)data);
}

/* System call: prctl */
long sys_prctl(long option, long arg2, long arg3, long arg4, long arg5, long unused1) {
    /* Process control */
    switch (option) {
        case PR_SET_PDEATHSIG:
            /* Set the parent death signal */
            return task_set_pdeathsig(task_current(), arg2);
        
        case PR_GET_PDEATHSIG:
            /* Get the parent death signal */
            return task_get_pdeathsig(task_current(), (int *)arg2);
        
        case PR_GET_DUMPABLE:
            /* Get the dumpable flag */
            return task_get_dumpable(task_current());
        
        case PR_SET_DUMPABLE:
            /* Set the dumpable flag */
            return task_set_dumpable(task_current(), arg2);
        
        case PR_GET_UNALIGN:
            /* Get the unaligned access mode */
            return task_get_unalign(task_current(), (int *)arg2);
        
        case PR_SET_UNALIGN:
            /* Set the unaligned access mode */
            return task_set_unalign(task_current(), arg2);
        
        case PR_GET_KEEPCAPS:
            /* Get the keep capabilities flag */
            return task_get_keepcaps(task_current());
        
        case PR_SET_KEEPCAPS:
            /* Set the keep capabilities flag */
            return task_set_keepcaps(task_current(), arg2);
        
        case PR_GET_FPEMU:
            /* Get the floating-point emulation mode */
            return task_get_fpemu(task_current(), (int *)arg2);
        
        case PR_SET_FPEMU:
            /* Set the floating-point emulation mode */
            return task_set_fpemu(task_current(), arg2);
        
        case PR_GET_FPEXC:
            /* Get the floating-point exception mode */
            return task_get_fpexc(task_current());
        
        case PR_SET_FPEXC:
            /* Set the floating-point exception mode */
            return task_set_fpexc(task_current(), arg2);
        
        case PR_GET_TIMING:
            /* Get the timing mode */
            return task_get_timing(task_current());
        
        case PR_SET_TIMING:
            /* Set the timing mode */
            return task_set_timing(task_current(), arg2);
        
        case PR_SET_NAME:
            /* Set the process name */
            return task_set_name(task_current(), (const char *)arg2);
        
        case PR_GET_NAME:
            /* Get the process name */
            return task_get_name(task_current(), (char *)arg2);
        
        case PR_GET_ENDIAN:
            /* Get the endian mode */
            return task_get_endian(task_current());
        
        case PR_SET_ENDIAN:
            /* Set the endian mode */
            return task_set_endian(task_current(), arg2);
        
        case PR_GET_SECCOMP:
            /* Get the seccomp mode */
            return task_get_seccomp(task_current());
        
        case PR_SET_SECCOMP:
            /* Set the seccomp mode */
            return seccomp_set_mode(arg2, arg3, (void *)arg4);
        
        case PR_CAPBSET_READ:
            /* Read the capability bounding set */
            return task_capbset_read(task_current(), arg2);
        
        case PR_CAPBSET_DROP:
            /* Drop a capability from the bounding set */
            return task_capbset_drop(task_current(), arg2);
        
        case PR_GET_TSC:
            /* Get the TSC mode */
            return task_get_tsc(task_current(), (int *)arg2);
        
        case PR_SET_TSC:
            /* Set the TSC mode */
            return task_set_tsc(task_current(), arg2);
        
        case PR_GET_SECUREBITS:
            /* Get the secure bits */
            return task_get_securebits(task_current());
        
        case PR_SET_SECUREBITS:
            /* Set the secure bits */
            return task_set_securebits(task_current(), arg2);
        
        case PR_SET_TIMERSLACK:
            /* Set the timer slack */
            return task_set_timerslack(task_current(), arg2);
        
        case PR_GET_TIMERSLACK:
            /* Get the timer slack */
            return task_get_timerslack(task_current());
        
        case PR_TASK_PERF_EVENTS_DISABLE:
            /* Disable performance events */
            return task_perf_events_disable(task_current());
        
        case PR_TASK_PERF_EVENTS_ENABLE:
            /* Enable performance events */
            return task_perf_events_enable(task_current());
        
        case PR_MCE_KILL:
            /* Set the MCE kill policy */
            return task_mce_kill(task_current(), arg2, arg3);
        
        case PR_MCE_KILL_GET:
            /* Get the MCE kill policy */
            return task_mce_kill_get(task_current());
        
        case PR_SET_MM:
            /* Set the memory map */
            return task_set_mm(task_current(), arg2, arg3, arg4, arg5);
        
        case PR_SET_CHILD_SUBREAPER:
            /* Set the child subreaper flag */
            return task_set_child_subreaper(task_current(), arg2);
        
        case PR_GET_CHILD_SUBREAPER:
            /* Get the child subreaper flag */
            return task_get_child_subreaper(task_current(), (int *)arg2);
        
        case PR_SET_NO_NEW_PRIVS:
            /* Set the no new privileges flag */
            return task_set_no_new_privs(task_current(), arg2);
        
        case PR_GET_NO_NEW_PRIVS:
            /* Get the no new privileges flag */
            return task_get_no_new_privs(task_current());
        
        case PR_GET_TID_ADDRESS:
            /* Get the TID address */
            return task_get_tid_address(task_current(), (int **)arg2);
        
        case PR_SET_THP_DISABLE:
            /* Set the THP disable flag */
            return task_set_thp_disable(task_current(), arg2);
        
        case PR_GET_THP_DISABLE:
            /* Get the THP disable flag */
            return task_get_thp_disable(task_current(), (int *)arg2);
        
        case PR_MPX_ENABLE_MANAGEMENT:
            /* Enable MPX management */
            return task_mpx_enable_management(task_current());
        
        case PR_MPX_DISABLE_MANAGEMENT:
            /* Disable MPX management */
            return task_mpx_disable_management(task_current());
        
        case PR_SET_FP_MODE:
            /* Set the FP mode */
            return task_set_fp_mode(task_current(), arg2);
        
        case PR_GET_FP_MODE:
            /* Get the FP mode */
            return task_get_fp_mode(task_current());
        
        case PR_CAP_AMBIENT:
            /* Ambient capability operations */
            return task_cap_ambient(task_current(), arg2, arg3);
        
        case PR_SVE_SET_VL:
            /* Set the SVE vector length */
            return task_sve_set_vl(task_current(), arg2);
        
        case PR_SVE_GET_VL:
            /* Get the SVE vector length */
            return task_sve_get_vl(task_current());
        
        case PR_GET_SPECULATION_CTRL:
            /* Get the speculation control */
            return task_get_speculation_ctrl(task_current(), arg2);
        
        case PR_SET_SPECULATION_CTRL:
            /* Set the speculation control */
            return task_set_speculation_ctrl(task_current(), arg2, arg3);
        
        case PR_PAC_RESET_KEYS:
            /* Reset the PAC keys */
            return task_pac_reset_keys(task_current(), arg2);
        
        case PR_SET_TAGGED_ADDR_CTRL:
            /* Set the tagged address control */
            return task_set_tagged_addr_ctrl(task_current(), arg2);
        
        case PR_GET_TAGGED_ADDR_CTRL:
            /* Get the tagged address control */
            return task_get_tagged_addr_ctrl(task_current());
        
        case PR_SET_IO_FLUSHER:
            /* Set the I/O flusher flag */
            return task_set_io_flusher(task_current(), arg2);
        
        case PR_GET_IO_FLUSHER:
            /* Get the I/O flusher flag */
            return task_get_io_flusher(task_current());
        
        default:
            return -1;
    }
    
    return -1;
}

/* System call: seccomp */
long sys_seccomp(long op, long flags, long uargs, long unused1, long unused2, long unused3) {
    /* Secure computing mode */
    return seccomp_set_mode(op, flags, (void *)uargs);
}

/* System call: getsid */
long sys_getsid(long pid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get the session ID */
    return task_getsid(pid);
}

/* System call: setsid */
long sys_setsid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Set the session ID */
    return task_setsid();
}

/* System call: getpgid */
long sys_getpgid(long pid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Get the process group ID */
    return task_getpgid(pid);
}

/* System call: setpgid */
long sys_setpgid(long pid, long pgid, long unused1, long unused2, long unused3, long unused4) {
    /* Set the process group ID */
    return task_setpgid(pid, pgid);
}

/* System call: getpgrp */
long sys_getpgrp(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Get the process group ID */
    return task_getpgrp();
}

/* System call: getuid */
long sys_getuid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Get the user ID */
    return task_getuid();
}

/* System call: geteuid */
long sys_geteuid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Get the effective user ID */
    return task_geteuid();
}

/* System call: getgid */
long sys_getgid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Get the group ID */
    return task_getgid();
}

/* System call: getegid */
long sys_getegid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    /* Get the effective group ID */
    return task_getegid();
}

/* System call: setuid */
long sys_setuid(long uid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Set the user ID */
    return task_setuid(uid);
}

/* System call: setgid */
long sys_setgid(long gid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Set the group ID */
    return task_setgid(gid);
}

/* System call: setreuid */
long sys_setreuid(long ruid, long euid, long unused1, long unused2, long unused3, long unused4) {
    /* Set the real and effective user IDs */
    return task_setreuid(ruid, euid);
}

/* System call: setregid */
long sys_setregid(long rgid, long egid, long unused1, long unused2, long unused3, long unused4) {
    /* Set the real and effective group IDs */
    return task_setregid(rgid, egid);
}

/* System call: setresuid */
long sys_setresuid(long ruid, long euid, long suid, long unused1, long unused2, long unused3) {
    /* Set the real, effective, and saved user IDs */
    return task_setresuid(ruid, euid, suid);
}

/* System call: getresuid */
long sys_getresuid(long ruid, long euid, long suid, long unused1, long unused2, long unused3) {
    /* Get the real, effective, and saved user IDs */
    return task_getresuid((uid_t *)ruid, (uid_t *)euid, (uid_t *)suid);
}

/* System call: setresgid */
long sys_setresgid(long rgid, long egid, long sgid, long unused1, long unused2, long unused3) {
    /* Set the real, effective, and saved group IDs */
    return task_setresgid(rgid, egid, sgid);
}

/* System call: getresgid */
long sys_getresgid(long rgid, long egid, long sgid, long unused1, long unused2, long unused3) {
    /* Get the real, effective, and saved group IDs */
    return task_getresgid((gid_t *)rgid, (gid_t *)egid, (gid_t *)sgid);
}

/* System call: setfsuid */
long sys_setfsuid(long fsuid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Set the file system user ID */
    return task_setfsuid(fsuid);
}

/* System call: setfsgid */
long sys_setfsgid(long fsgid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    /* Set the file system group ID */
    return task_setfsgid(fsgid);
}

/* System call: getgroups */
long sys_getgroups(long size, long list, long unused1, long unused2, long unused3, long unused4) {
    /* Get the supplementary group IDs */
    return task_getgroups(size, (gid_t *)list);
}

/* System call: setgroups */
long sys_setgroups(long size, long list, long unused1, long unused2, long unused3, long unused4) {
    /* Set the supplementary group IDs */
    return task_setgroups(size, (const gid_t *)list);
}

/* Register security system calls */
void security_syscalls_init(void) {
    /* Register security system calls */
    syscall_register(SYS_CAPGET, sys_capget);
    syscall_register(SYS_CAPSET, sys_capset);
    syscall_register(SYS_PRCTL, sys_prctl);
    syscall_register(SYS_SECCOMP, sys_seccomp);
    syscall_register(SYS_GETSID, sys_getsid);
    syscall_register(SYS_SETSID, sys_setsid);
    syscall_register(SYS_GETPGID, sys_getpgid);
    syscall_register(SYS_SETPGID, sys_setpgid);
    syscall_register(SYS_GETPGRP, sys_getpgrp);
    syscall_register(SYS_GETUID, sys_getuid);
    syscall_register(SYS_GETEUID, sys_geteuid);
    syscall_register(SYS_GETGID, sys_getgid);
    syscall_register(SYS_GETEGID, sys_getegid);
    syscall_register(SYS_SETUID, sys_setuid);
    syscall_register(SYS_SETGID, sys_setgid);
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
}
