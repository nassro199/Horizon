/**
 * sysinfo.c - Horizon kernel system information-related system calls
 *
 * This file contains the implementation of system information-related system calls.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/syscall.h>
#include <horizon/sysinfo.h>
#include <horizon/task.h>
#include <horizon/mm.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* System info system call */
long sys_sysinfo(long info, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Get system information */
    struct sysinfo *si = (struct sysinfo *)info;

    /* Check if the sysinfo structure is valid */
    if (si == NULL) {
        return -EINVAL;
    }

    /* Fill in the sysinfo structure */
    si->uptime = time_get_uptime();
    si->loads[0] = sched_get_load_avg(1);
    si->loads[1] = sched_get_load_avg(5);
    si->loads[2] = sched_get_load_avg(15);
    si->totalram = mm_get_total_ram();
    si->freeram = mm_get_free_ram();
    si->sharedram = mm_get_shared_ram();
    si->bufferram = mm_get_buffer_ram();
    si->totalswap = mm_get_total_swap();
    si->freeswap = mm_get_free_swap();
    si->procs = task_get_process_count();
    si->totalhigh = mm_get_total_high();
    si->freehigh = mm_get_free_high();
    si->mem_unit = 1;

    return 0;
}

/* Uname system call */
long sys_uname(long name, long arg2, long arg3, long arg4, long arg5, long arg6) {
    /* Get system information */
    struct utsname *buf = (struct utsname *)name;

    /* Check if the utsname structure is valid */
    if (buf == NULL) {
        return -EINVAL;
    }

    /* Fill in the utsname structure */
    strcpy(buf->sysname, "Horizon");
    strcpy(buf->nodename, "localhost");
    strcpy(buf->release, "1.0.0");
    strcpy(buf->version, "1.0.0");
    strcpy(buf->machine, "x86_64");
    strcpy(buf->domainname, "");

    return 0;
}

/* Get hostname system call */
long sys_gethostname(long name, long len, long arg3, long arg4, long arg5, long arg6) {
    /* Get the hostname */
    char *buf = (char *)name;
    size_t buflen = len;

    /* Check if the buffer is valid */
    if (buf == NULL) {
        return -EINVAL;
    }

    /* Get the hostname */
    const char *hostname = "localhost";
    size_t hostname_len = strlen(hostname);

    /* Copy the hostname to the buffer */
    if (buflen < hostname_len + 1) {
        return -ENAMETOOLONG;
    }

    strcpy(buf, hostname);

    return 0;
}

/* Set hostname system call */
long sys_sethostname(long name, long len, long arg3, long arg4, long arg5, long arg6) {
    /* Set the hostname */
    const char *buf = (const char *)name;
    size_t buflen = len;

    /* Check if the buffer is valid */
    if (buf == NULL) {
        return -EINVAL;
    }

    /* Check if the user has permission to set the hostname */
    if (task_current()->euid != 0) {
        return -EPERM;
    }

    /* Check if the hostname is too long */
    if (buflen > 64) {
        return -ENAMETOOLONG;
    }

    /* Set the hostname */
    /* This would be implemented with a proper hostname storage */
    /* For now, just return success */

    return 0;
}

/* Get domain name system call */
long sys_getdomainname(long name, long len, long arg3, long arg4, long arg5, long arg6) {
    /* Get the domain name */
    char *buf = (char *)name;
    size_t buflen = len;

    /* Check if the buffer is valid */
    if (buf == NULL) {
        return -EINVAL;
    }

    /* Get the domain name */
    const char *domainname = "";
    size_t domainname_len = strlen(domainname);

    /* Copy the domain name to the buffer */
    if (buflen < domainname_len + 1) {
        return -ENAMETOOLONG;
    }

    strcpy(buf, domainname);

    return 0;
}

/* Set domain name system call */
long sys_setdomainname(long name, long len, long arg3, long arg4, long arg5, long arg6) {
    /* Set the domain name */
    const char *buf = (const char *)name;
    size_t buflen = len;

    /* Check if the buffer is valid */
    if (buf == NULL) {
        return -EINVAL;
    }

    /* Check if the user has permission to set the domain name */
    if (task_current()->euid != 0) {
        return -EPERM;
    }

    /* Check if the domain name is too long */
    if (buflen > 64) {
        return -ENAMETOOLONG;
    }

    /* Set the domain name */
    /* This would be implemented with a proper domain name storage */
    /* For now, just return success */

    return 0;
}

/* Initialize system information-related system calls */
void sysinfo_syscalls_init(void) {
    /* Register system information-related system calls */
    syscall_register(SYS_SYSINFO, sys_sysinfo);
    syscall_register(SYS_UNAME, sys_uname);
    syscall_register(SYS_GETHOSTNAME, sys_gethostname);
    syscall_register(SYS_SETHOSTNAME, sys_sethostname);
    syscall_register(SYS_GETDOMAINNAME, sys_getdomainname);
    syscall_register(SYS_SETDOMAINNAME, sys_setdomainname);
}
